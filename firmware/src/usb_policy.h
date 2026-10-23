#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum { KS_STARTUP_DETECT, KS_BACKUP_EXCLUSIVE, KS_USB_READONLY, KS_SAFE_SHUTDOWN } ks_mode_t;
typedef struct { ks_mode_t mode; bool sd_mounted_by_app; bool usb_exposed; bool operation_in_progress; uint64_t startup_deadline_ms; } ks_usb_policy_t;
void ks_policy_init(ks_usb_policy_t *p,uint64_t now_ms,uint32_t detection_window_ms);
bool ks_policy_host_detected(ks_usb_policy_t *p);
bool ks_policy_detection_expired(ks_usb_policy_t *p,uint64_t now_ms);
bool ks_policy_app_mount(ks_usb_policy_t *p);
bool ks_policy_app_unmount(ks_usb_policy_t *p);
bool ks_policy_begin_write(ks_usb_policy_t *p);
void ks_policy_end_write(ks_usb_policy_t *p);
bool ks_policy_expose_usb(ks_usb_policy_t *p);
void ks_policy_shutdown(ks_usb_policy_t *p);
