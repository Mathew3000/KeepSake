#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "tusb.h"
#include "sdmmc_cmd.h"

/*
 * EXPERIMENTAL read-only MSC backend. Not yet linked into the firmware.
 * Host block access only after exclusive FAT unmount and exclusive card ownership.
 * Must pair with a descriptor advertising USB MSC interface and TinyUSB driver.
 * Compile as an exclusive alternative to esp_tinyusb's built-in writable MSC backend.
 */
static sdmmc_card_t *s_card = NULL;
void ks_msc_attach_card(sdmmc_card_t *card) { s_card=card; }
void tud_msc_inquiry_cb(uint8_t lun,uint8_t vendor[8],uint8_t product[16],uint8_t rev[4]){
 (void)lun;memcpy(vendor,"KEEPSAKE",8);memcpy(product,"EMERGENCY BACKUP",16);memcpy(rev,"0001",4);
}
bool tud_msc_test_unit_ready_cb(uint8_t lun){(void)lun;return s_card!=NULL;}
void tud_msc_capacity_cb(uint8_t lun,uint32_t *blocks,uint16_t *block_size){
 (void)lun;*blocks=s_card?s_card->csd.capacity:0;*block_size=s_card?s_card->csd.sector_size:512;
}
bool tud_msc_is_writable_cb(uint8_t lun){(void)lun;return false;}
bool tud_msc_start_stop_cb(uint8_t lun,uint8_t power_condition,bool start,bool load_eject){
 (void)lun;(void)power_condition;(void)start;(void)load_eject;return true;
}
int32_t tud_msc_read10_cb(uint8_t lun,uint32_t lba,uint32_t offset,void *buffer,uint32_t len){
 (void)lun;
 if(!s_card || !buffer || !len || s_card->csd.sector_size!=512 || offset>=512 || len>512-offset || lba>=s_card->csd.capacity){
   tud_msc_set_sense(lun,SCSI_SENSE_ILLEGAL_REQUEST,0x21,0);return -1;
 }
 uint8_t block[512];
 if(sdmmc_read_sectors(s_card,block,lba,1)!=ESP_OK){
   tud_msc_set_sense(lun,SCSI_SENSE_MEDIUM_ERROR,0x11,0);return -1;
 }
 memcpy(buffer,block+offset,len);return (int32_t)len;
}
int32_t tud_msc_write10_cb(uint8_t lun,uint32_t lba,uint32_t offset,uint8_t *buffer,uint32_t len){
 (void)lba;(void)offset;(void)buffer;(void)len;
 tud_msc_set_sense(lun,SCSI_SENSE_DATA_PROTECT,0x27,0);return -1;
}
int32_t tud_msc_scsi_cb(uint8_t lun,uint8_t const scsi_cmd[16],void *buffer,uint16_t bufsize){
 (void)scsi_cmd;(void)buffer;(void)bufsize;
 tud_msc_set_sense(lun,SCSI_SENSE_ILLEGAL_REQUEST,0x20,0);return -1;
}
