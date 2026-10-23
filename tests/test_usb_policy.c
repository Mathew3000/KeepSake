#include "usb_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 ks_usb_policy_t p;ks_policy_init(&p,0,5000);
 assert(!ks_policy_app_mount(&p));assert(!ks_policy_detection_expired(&p,4999));
 assert(ks_policy_host_detected(&p));assert(ks_policy_expose_usb(&p));
 assert(!ks_policy_app_mount(&p));assert(!ks_policy_begin_write(&p));
 ks_policy_init(&p,0,5000);assert(ks_policy_detection_expired(&p,5000));
 assert(ks_policy_app_mount(&p));assert(ks_policy_begin_write(&p));
 assert(!ks_policy_app_unmount(&p));ks_policy_end_write(&p);
 assert(ks_policy_app_unmount(&p));assert(!ks_policy_expose_usb(&p));
 puts("USB exclusive ownership policy passed");return 0;
}
