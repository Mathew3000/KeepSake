# First hardware acceptance plan

Use **synthetic encrypted test data**, not the real KeePass database, until all checks pass.

## Flash and test
1. Clone the feature branch, then `cd firmware`.
2. Create `main/settings.h` from `main/settings.example.h`, fill test Wi-Fi credentials, device ID and `https://host:port/backup`.
3. Create `main/certs/server-ca.pem`, `device.crt`, `device.key` with test certificates.
4. Run `pio run` first. If it fails, attach the complete compiler output to PR #2.
5. Flash using the board's programming/serial port, then inspect serial at 115200 baud.
6. Insert FAT-formatted microSD. A successful cycle should make `/history/<sha>.bundle`, `/reference.bundle`, and `/START_HIER.html`.
7. Load the media in a separate computer and independently verify hash against the server manifest. Recover a KeePass test database from a copied `.bundle` renamed `.kdbx`.

## Failure injection
- Wrong server cert: must refuse TLS.
- Wrong client cert: server must refuse connection.
- Bad / truncated object: no new committed snapshot.
- Repeat unchanged payload: no new version.
- Publish 21 different snapshots: retain most recent 20, never delete `reference.bundle`.
- Remove network, corrupt manifest, fill SD, hard power cut during download: previous verified versions must remain.
- Trigger heartbeat failure and check monitoring signal.
- Connect laptop to USB_DEV Type-A port: do **not** expect a working MSC device until experimental code is integrated and USB write protection verified.
- After future USB MSC integration, test Linux/Windows/macOS, attempt to write/format, and confirm write-protect failure.

## Risk register
- FAT rename and directory updates cannot guarantee power-loss atomicity; remove card only after unmount, and keep separate offline copies.
- First-time reference seeding may capture a bad-but-hash-valid application payload; verify restoration, not just SHA-256.
- Signing an OTA manifest alone is insufficient. Verify signature in firmware before any installer writes OTA partitions.
- Secure Boot and Flash Encryption are not yet enabled. Certificates embedded in flash should be considered extractable until they are.
- Default one-port behavior is not implemented; currently the board runs backups and sleeps.
