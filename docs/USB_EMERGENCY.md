# USB emergency mode: design and hardware integration

## What is implemented
`usb_policy.c` provides a tested **exclusive SD ownership** state machine:
- after boot, wait for actual USB host enumeration (not merely 5 V VBUS);
- if a PC enumerates the device, stop normal backup and expose storage read-only;
- otherwise, enter backup mode, mount FAT to the app, perform one sync and sleep;
- the program refuses concurrent USB exposure and SD writes.

The policy is tested with a host C compiler in CI, but **it is not yet connected to TinyUSB hardware events or the ESP-IDF SD driver**. At the moment the binary does not present a USB drive.

## Board details
On ESP32-S3-USB-OTG, `USB_DEV` is the male Type-A connector. GPIO18 (USB_SEL) must remain low to route the ESP32-S3 USB peripheral to this port. The micro-USB connector is the USB-to-UART/programming port, *not* the intended emergency MSC data path.

The board supports SDMMC and a Li-ion battery charge circuit. The schematic and correct SD pin mapping must be checked during the physical test.

## Host mode integration acceptance criteria
1. Attach the male USB-A device port through a **proper data-capable USB adapter** to a computer.
2. Enumerate a Mass Storage Class (MSC) volume; reject SCSI write operations (write-protected sense code).
3. SD FAT can be owned by **either** the host **or** the app, never concurrently.
4. While host is connected: WLAN and sync are stopped, and OTA is disabled.
5. While powered by a charge-only USB adapter: no host enumeration after a bounded grace period; perform normal sync.
6. Reconnecting/rebooting during partial transfers never removes the last verified snapshot.
7. Test Windows/macOS/Linux host write attempts; host should clearly report write protection.
8. If USB is unusable, user can remove SD and read with a standard card reader.

## Important design decision
Espressif's TinyUSB MSC helper can expose SD media but **read-only must be enforced in the underlying MSC write callback**. No read-only mass storage is being claimed until that path is implemented and tested. Formatting by the USB host must also be refused.

A 5-second detection timeout is a starting point, not a verified USB negotiation value. A single-port power/data arrangement still requires physical validation, especially when powered from a USB charger rather than a PC.

Documentation: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-otg/user_guide.html
