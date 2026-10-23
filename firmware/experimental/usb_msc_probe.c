/*
 * Hardware-only TinyUSB 2.x integration probe. Intentionally NOT linked into
 * production firmware until board enumeration and read-only MSC are validated.
 *
 * Important: reporting USB_EVENT_ATTACHED alone is NOT enough to expose SD.
 * Configure MSC as a write-protected LUN before enabling normal emergency mode.
 */
#include <stdbool.h>
#include "esp_err.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

static volatile bool computer_enumerated;

static void device_event_handler(tinyusb_event_t *ev, void *context) {
    (void)context;
    if (ev->id == TINYUSB_EVENT_ATTACHED) computer_enumerated = true;
    if (ev->id == TINYUSB_EVENT_DETACHED) computer_enumerated = false;
}

esp_err_t keepsake_usb_probe_start(void) {
    tinyusb_config_t config = TINYUSB_DEFAULT_CONFIG(device_event_handler);
    return tinyusb_driver_install(&config);
}
bool keepsake_usb_host_present(void) { return computer_enumerated; }
esp_err_t keepsake_usb_probe_stop(void) { return tinyusb_driver_uninstall(); }
