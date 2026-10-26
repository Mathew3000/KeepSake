#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_sntp.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"
#include "settings.h"
#include "usb_policy.h"

#define SD "/sdcard"
#define HISTORY SD "/history"
#define REFERENCE SD "/reference.bundle"
#define GUIDE SD "/START_HIER.html"
#define TEMP HISTORY "/incoming.part"
#define MAX_HISTORY 20
#define MANIFEST_MAX 1024
#define MAX_HISTORY_SCAN 256
#define STR2(x) #x
#define STR(x) STR2(x)

static const char *TAG="KeepSake";
static EventGroupHandle_t wifi_evt;
static const EventBits_t WIFI_OK=BIT0, WIFI_ERR=BIT1;
extern const uint8_t _binary_certs_server_ca_pem_start[];
extern const uint8_t _binary_certs_device_crt_start[];
extern const uint8_t _binary_certs_device_key_start[];

typedef struct { char hash[65]; char filename[72]; int64_t size; } manifest_t;
static bool hex64(const char *s) {
    if(!s || strlen(s)!=64) return false;
    for(int i=0;i<64;i++) if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;
    return true;
}
static void hashhex(const unsigned char b[32],char out[65]) {
    static const char *hex="0123456789abcdef";
    for(int i=0;i<32;i++){out[2*i]=hex[b[i]>>4];out[2*i+1]=hex[b[i]&15];}
    out[64]=0;
}
static void wifi_events(void *ctx, esp_event_base_t base, int32_t id, void *event) {
    static unsigned retry=0;
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START) {esp_wifi_connect();}
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        if(retry++ < 8) esp_wifi_connect();
        else xEventGroupSetBits(wifi_evt,WIFI_ERR);
    } else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        retry=0;xEventGroupSetBits(wifi_evt,WIFI_OK);
    }
}
static esp_err_t wifi_connect(void) {
    wifi_evt=xEventGroupCreate();if(!wifi_evt)return ESP_ERR_NO_MEM;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_events,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_events,NULL));
    wifi_config_t config={0};
    strncpy((char*)config.sta.ssid,KS_WIFI_SSID,sizeof(config.sta.ssid)-1);
    strncpy((char*)config.sta.password,KS_WIFI_PASSWORD,sizeof(config.sta.password)-1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&config));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t b=xEventGroupWaitBits(wifi_evt,WIFI_OK|WIFI_ERR,pdFALSE,pdFALSE,pdMS_TO_TICKS(30000));
    return (b&WIFI_OK)?ESP_OK:ESP_FAIL;
}
static bool get_time(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0,"pool.ntp.org");
    esp_sntp_init();
    for(int i=0;i<40;i++) {
        if(time(NULL)>1767225600) return true; // 2026-01-01
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return false;
}
static esp_err_t sd_mount(void){
    sdmmc_host_t host=SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width=1;
    slot.clk=KS_SD_CLK;slot.cmd=KS_SD_CMD;slot.d0=KS_SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_vfs_fat_sdmmc_mount_config_t cfg={.format_if_mount_failed=false,.max_files=5,.allocation_unit_size=16384};
    sdmmc_card_t *card=NULL;
    esp_err_t e=esp_vfs_fat_sdmmc_mount(SD,&host,&slot,&cfg,&card);
    if(e==ESP_OK && mkdir(HISTORY,0755)!=0 && errno!=EEXIST) return ESP_FAIL;
    return e;
}
static esp_http_client_handle_t http(const char *url,esp_http_client_method_t method) {
    esp_http_client_config_t cfg={
        .url=url, .method=method,
        .cert_pem=(const char *)_binary_certs_server_ca_pem_start,
        .client_cert_pem=(const char *)_binary_certs_device_crt_start,
        .client_key_pem=(const char *)_binary_certs_device_key_start,
        .timeout_ms=20000, .disable_auto_redirect=true
    };
    return esp_http_client_init(&cfg);
}
static bool open_ok(esp_http_client_handle_t c,int expected_status) {
    if(esp_http_client_open(c,0)!=ESP_OK)return false;
    if(esp_http_client_fetch_headers(c)<0)return false;
    return esp_http_client_get_status_code(c)==expected_status;
}
static bool manifest_fetch(manifest_t *m) {
    char url[300];snprintf(url,sizeof(url),"%s/latest.json",KS_BACKUP_BASE_URL);
    esp_http_client_handle_t c=http(url,HTTP_METHOD_GET);if(!c)return false;
    bool ok=false; char buf[MANIFEST_MAX+1];size_t total=0;
    if(!open_ok(c,200))goto done;
    int64_t length=esp_http_client_get_content_length(c);
    if(length<=0 || length>MANIFEST_MAX)goto done;
    while(total<(size_t)length) {
        int n=esp_http_client_read(c,buf+total,(int)((size_t)length-total));
        if(n<=0)goto done;
        total+=(size_t)n;
    }
    buf[total]=0;
    cJSON *root=cJSON_ParseWithLength(buf,total);
    if(!root)goto done;
    cJSON *schema=cJSON_GetObjectItemCaseSensitive(root,"schema");
    cJSON *sha=cJSON_GetObjectItemCaseSensitive(root,"sha256");
    cJSON *name=cJSON_GetObjectItemCaseSensitive(root,"filename");
    cJSON *size=cJSON_GetObjectItemCaseSensitive(root,"size");
    if(cJSON_IsNumber(schema) && schema->valueint==1 &&
       cJSON_IsString(sha) && hex64(sha->valuestring) &&
       cJSON_IsString(name) && cJSON_IsNumber(size) &&
       size->valuedouble>=1 && size->valuedouble<=KS_MAX_BUNDLE_BYTES &&
       size->valuedouble==(int64_t)size->valuedouble) {
        char expected[72];
        snprintf(expected,sizeof(expected),"%s.bundle",sha->valuestring);
        if(strcmp(name->valuestring,expected)==0){
            strcpy(m->hash,sha->valuestring);
            strcpy(m->filename,expected);
            m->size=(int64_t)size->valuedouble;ok=true;
        }
    }
    cJSON_Delete(root);
done:
    esp_http_client_cleanup(c);return ok;
}
static bool checksum_file(const char *path,const char *expected,int64_t size){
    FILE *f=fopen(path,"rb");if(!f)return false;
    mbedtls_sha256_context c;mbedtls_sha256_init(&c);mbedtls_sha256_starts(&c,0);
    unsigned char buf[2048];size_t n;int64_t total=0;
    while((n=fread(buf,1,sizeof(buf),f))>0){mbedtls_sha256_update(&c,buf,n);total+=n;}
    bool ok=(!ferror(f) && total==size);
    fclose(f);
    unsigned char digest[32];char hex[65];
    mbedtls_sha256_finish(&c,digest);mbedtls_sha256_free(&c);
    hashhex(digest,hex);return ok && strcmp(hex,expected)==0;
}
static bool download(manifest_t *m){
    char dest[160];snprintf(dest,sizeof(dest),HISTORY "/%s",m->filename);
    if(access(dest,F_OK)==0) {
        if(checksum_file(dest,m->hash,m->size))return true;
        ESP_LOGE(TAG,"Existing snapshot corrupted; preserving it for investigation");
        return false;
    }
    char url[300];snprintf(url,sizeof(url),"%s/%s",KS_BACKUP_BASE_URL,m->filename);
    esp_http_client_handle_t c=http(url,HTTP_METHOD_GET);if(!c)return false;
    bool success=false;FILE *f=NULL;
    if(!open_ok(c,200))goto done;
    if(esp_http_client_get_content_length(c)!=m->size)goto done;
    f=fopen(TEMP,"wb");if(!f)goto done;
    unsigned char buf[2048];int64_t total=0;
    while(total<m->size){
        int request=(m->size-total)<sizeof(buf)?(int)(m->size-total):(int)sizeof(buf);
        int n=esp_http_client_read(c,(char*)buf,request);
        if(n<=0 || fwrite(buf,1,n,f)!=(size_t)n)goto done;
        total+=n;
    }
    if(fflush(f)!=0 || fsync(fileno(f))!=0)goto done;
    if(fclose(f)!=0){f=NULL;goto done;} f=NULL;
    if(!checksum_file(TEMP,m->hash,m->size))goto done;
    /* FAT rename alone is not a power-loss transaction. Existing snapshots stay untouched. */
    if(rename(TEMP,dest)!=0)goto done;
    success=true;
done:
    if(f)fclose(f);
    if(!success)unlink(TEMP);
    esp_http_client_cleanup(c);
    return success;
}
typedef struct {char name[72];time_t mtime;} entry_t;
/* Manual reference is independent of history rotation.
 * First successful verified download seeds it; never automatically overwrites.
 * Physical access and destructive formatting remain attack vectors.
 */
static void seed_reference(const manifest_t *m) {
    if (access(REFERENCE,F_OK)==0) return;
    char source[160];
    snprintf(source,sizeof(source),HISTORY "/%s",m->filename);
    FILE *in=fopen(source,"rb");
    if(!in)return;
    FILE *out=fopen(SD "/reference.pending","wb");
    if(!out){fclose(in);return;}
    unsigned char block[2048];size_t n;bool good=true;
    while((n=fread(block,1,sizeof(block),in))>0){
        if(fwrite(block,1,n,out)!=n){good=false;break;}
    }
    if(ferror(in)||fflush(out)!=0||fsync(fileno(out))!=0)good=false;
    fclose(in);
    if(fclose(out)!=0)good=false;
    if(!good || !checksum_file(SD "/reference.pending",m->hash,m->size) || rename(SD "/reference.pending",REFERENCE)!=0) {
        unlink(SD "/reference.pending");
        ESP_LOGE(TAG,"Reference seeding failed");
    }
}
static const char emergency_guide[] =
    "<!doctype html><html lang=\"de\"><meta charset=\"utf-8\">"
    "<title>KeepSake - Hier anfangen</title><body>"
    "<h1>KeepSake: Notfallzugriff</h1>"
    "<p>Bitte den versiegelten Notfallbrief nehmen und die angegebene Vertrauensperson anrufen.</p>"
    "<p>Backups liegen im Ordner <code>history</code> und sind verschluesselt.</p>"
    "<p>Erst Dateien auf einen Computer kopieren. Nichts formatieren oder loeschen.</p>"
    "<p>Mit KeePassXC/KeePass und dem separaten Passwort oeffnen. "
    "Falls eine Schluesseldatei existiert, wird diese ebenfalls benoetigt.</p>"
    "</body></html>";
static void seed_guide(void) {
    if(access(GUIDE,F_OK)==0)return;
    FILE *f=fopen(GUIDE,"wb");if(!f)return;
    size_t len=strlen(emergency_guide);
    if(fwrite(emergency_guide,1,len,f)!=len || fflush(f)!=0 || fsync(fileno(f))!=0)
        ESP_LOGE(TAG,"Writing emergency guide failed");
    fclose(f);
}
static void rotate(void){
    DIR *d=opendir(HISTORY);if(!d)return;
    entry_t a[MAX_HISTORY_SCAN];size_t n=0;struct dirent *entry;
    while((entry=readdir(d))!=NULL){
        const char *name=entry->d_name;
        if(strlen(name)!=71 || strcmp(name+64,".bundle")!=0)continue;
        char hash[65];memcpy(hash,name,64);hash[64]=0;
        if(!hex64(hash))continue;
        if(n==MAX_HISTORY_SCAN){ESP_LOGW(TAG,"History scan capacity reached; skipping rotation");closedir(d);return;}
        char path[160];snprintf(path,sizeof(path),HISTORY "/%.71s",name);
        struct stat st;if(stat(path,&st)!=0)continue;
        strcpy(a[n].name,name);a[n].mtime=st.st_mtime;n++;
    }closedir(d);
    while(n>MAX_HISTORY){
        size_t oldest=0;
        for(size_t i=1;i<n;i++)if(a[i].mtime<a[oldest].mtime)oldest=i;
        char path[160];snprintf(path,sizeof(path),HISTORY "/%s",a[oldest].name);
        if(unlink(path)!=0)break;
        a[oldest]=a[--n];
    }
}
static void heartbeat(const char *hash,const char *result){
    char url[300];snprintf(url,sizeof(url),"%s/heartbeat",KS_BACKUP_BASE_URL);
    esp_http_client_handle_t c=http(url,HTTP_METHOD_POST);if(!c)return;
    char json[200];snprintf(json,sizeof(json),
        "{\"device\":\"%s\",\"sha256\":\"%s\",\"result\":\"%s\"}",
        KS_DEVICE_ID,hash,result);
    esp_http_client_set_header(c,"Content-Type","application/json");
    esp_http_client_set_post_field(c,json,strlen(json));
    if(esp_http_client_perform(c)!=ESP_OK || esp_http_client_get_status_code(c)!=204)
        ESP_LOGW(TAG,"Heartbeat not accepted (requires server endpoint)");
    esp_http_client_cleanup(c);
}
extern bool keepsake_detect_usb_host(uint32_t window_ms);
void app_main(void){
    if(keepsake_detect_usb_host(5000)){
        /* Fail closed: never allow backup writes while a PC is connected. */
        while(true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    esp_err_t e=nvs_flash_init();
    if(e==ESP_ERR_NVS_NO_FREE_PAGES||e==ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());e=nvs_flash_init();
    }ESP_ERROR_CHECK(e);
    bool storage=(sd_mount()==ESP_OK);
    bool network=(wifi_connect()==ESP_OK);
    bool valid_time=network&&get_time();
    manifest_t manifest={0};
    bool verified=false;
    if(storage) seed_guide();
    if(storage && valid_time && manifest_fetch(&manifest)) {
        verified=download(&manifest);
        if(verified){seed_reference(&manifest);rotate();}
    }
    if(valid_time)heartbeat(manifest.hash,verified?"success":"failure");
    ESP_LOGI(TAG,"Backup cycle: %s",verified?"verified":"failed");
    esp_sleep_enable_timer_wakeup(KS_SYNC_SECONDS*1000000ULL);
    esp_deep_sleep_start();
}
