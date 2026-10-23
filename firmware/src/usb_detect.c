#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include <stdbool.h>
static volatile bool ks_usb_attached=false;
static void device_event(tinyusb_event_t *event, void *ctx) {
    (void)ctx;
    if(event->id==TINYUSB_EVENT_ATTACHED)ks_usb_attached=true;
    else if(event->id==TINYUSB_EVENT_DETACHED)ks_usb_attached=false;
}
/* Boot-time host detection. Default USB class is a probe, NOT MSC storage.
 * Returns true only after actual USB enumeration. Do not run backup in
 * this state; the MSC interface must be integrated and hardware qualified.
 */
bool keepsake_detect_usb_host(uint32_t window_ms) {
    gpio_reset_pin(GPIO_NUM_18);
    gpio_set_direction(GPIO_NUM_18,GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18,0); /* USB_DEV Type-A male port */
    tinyusb_config_t cfg=TINYUSB_DEFAULT_CONFIG(device_event);
    if(tinyusb_driver_install(&cfg)!=ESP_OK){
        ESP_LOGW("USB","TinyUSB install failed; no host detection");
        return false;
    }
    for(uint32_t waited=0;waited<window_ms;waited+=100){
        if(ks_usb_attached){
            ESP_LOGW("USB","Computer enumerated: backup is disabled; MSC not yet enabled");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    tinyusb_driver_uninstall();
    return false;
}
