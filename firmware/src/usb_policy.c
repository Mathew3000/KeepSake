#include "usb_policy.h"
#include <stddef.h>
void ks_policy_init(ks_usb_policy_t *p,uint64_t now_ms,uint32_t window_ms){if(p)*p=(ks_usb_policy_t){.mode=KS_STARTUP_DETECT,.startup_deadline_ms=now_ms+window_ms};}
bool ks_policy_host_detected(ks_usb_policy_t *p){if(!p||p->mode!=KS_STARTUP_DETECT||p->sd_mounted_by_app||p->operation_in_progress)return false;p->mode=KS_USB_READONLY;return true;}
bool ks_policy_detection_expired(ks_usb_policy_t *p,uint64_t now_ms){if(!p||p->mode!=KS_STARTUP_DETECT||now_ms<p->startup_deadline_ms)return false;p->mode=KS_BACKUP_EXCLUSIVE;return true;}
bool ks_policy_app_mount(ks_usb_policy_t *p){if(!p||p->mode!=KS_BACKUP_EXCLUSIVE||p->usb_exposed||p->sd_mounted_by_app)return false;p->sd_mounted_by_app=true;return true;}
bool ks_policy_app_unmount(ks_usb_policy_t *p){if(!p||!p->sd_mounted_by_app||p->operation_in_progress)return false;p->sd_mounted_by_app=false;return true;}
bool ks_policy_begin_write(ks_usb_policy_t *p){if(!p||p->mode!=KS_BACKUP_EXCLUSIVE||!p->sd_mounted_by_app||p->usb_exposed||p->operation_in_progress)return false;p->operation_in_progress=true;return true;}
void ks_policy_end_write(ks_usb_policy_t *p){if(p)p->operation_in_progress=false;}
bool ks_policy_expose_usb(ks_usb_policy_t *p){if(!p||p->mode!=KS_USB_READONLY||p->sd_mounted_by_app||p->operation_in_progress)return false;p->usb_exposed=true;return true;}
void ks_policy_shutdown(ks_usb_policy_t *p){if(p)p->mode=KS_SAFE_SHUTDOWN;}
