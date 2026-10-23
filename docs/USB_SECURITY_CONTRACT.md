# KeepSake MCU security & recovery contract

The intended USB emergency mode is a **read-only** mass-storage device, never a writable FAT partition.

## Ownership
1. On start USB device detection runs before the application mounts its SD card.
2. Once USB host enumeration is detected, **never** mount the filesystem in the backup application.
3. A SCSI WRITE(10)/WRITE(12)/WRITE(16) request must return DATA PROTECT/WRITE PROTECTED (sense key 0x07, ASC 0x27, ASCQ 0x00). FORMAT UNIT and UNMAP must likewise be rejected.
4. MSC INQUIRY/MODE SENSE must advertise write protection, and TEST UNIT READY must not report ready until SD is mounted exclusively for MSC.
5. No OTA, heartbeat file writes, or housekeeping during active USB mode.
6. If host detection times out, teardown the USB device driver before mounting SD for backups. A host connecting *later* must not hot-switch storage mid-write; it can safely wait until the next restart.
7. If SD initialization fails, fail closed; never expose a partially initialized block device.

## IMPORTANT
The current TinyUSB probe and USB policy do **not** provide a working USB disk. An SD card should never be exposed with the default TinyUSB MSC storage module until its write callbacks have been audited or overridden to be read-only. Even a 'read-only' FAT mount in ESP-IDF does not prevent a USB block storage host from writing.

## SD recovery
Keep the microSD removable. Users can remove it and insert in a computer using a standard USB card reader while USB firmware is untested. Copy files elsewhere before opening them.

## Source documentation
- https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/device/tusb_msc
- https://github.com/espressif/esp-usb/tree/master/device/esp_tinyusb
