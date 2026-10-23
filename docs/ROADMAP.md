# Hardware-independent progress and remaining blockers

## What is committed
- Wi-Fi + NTP, mTLS GET, SHA-256 readback and content-addressed snapshots
- 20-version rotation, SQLite heartbeats, health CLI and certificate issuance
- Reference pinning utility and Python automated server tests
- Exclusive SD ownership policy for USB emergency mode (host-compiled and tested)
- Experimental TinyUSB host-enumeration probe; isolated from main firmware
- GitHub Actions checks: Python tests, native C policy test and PlatformIO build attempt

## Acceptance criteria for actual USB emergency mode
**NOT YET IMPLEMENTED**: Mass Storage Class SD exposure and write protection.
This is an explicitly safety-critical integration point. No code should expose a writable SD to a PC while the app holds the FAT mount. The firmware should detect actual USB host enumeration, not just the 5-V cable, and only publish a read-only MSC device. The board uses the Type-A male USB_DEV connector; USB_SEL GPIO18 is low for device mode.

## Remaining feature gaps
- Real USB MSC read-only LUN and host-detect handoff (board test essential)
- Full embedded firmware compilation and hardware tests
- Certificate revocation: NGINX fingerprint allowlist generator is implemented; operational deployment, atomic reload and rotation still need validation
- Device-side separately protected reference snapshot (manual server reference works)
- Notification delivery (health-check command can be wired into monitoring)
- Server-side RSA-PSS OTA manifest signing is implemented; device-side signature checking, installation and rollback are pending
- Signed manifest / trust anchor provisioning (mTLS currently authenticates transport)
- End-to-end recovery rehearsal, including backup keyfiles and encrypted documentation

## Security requirements
- Never embed a CA signing private key or actual KeePass master password in firmware.
- Do not publish plaintext recovery instructions containing private credentials.
- Keep firmware and backup snapshots independently restorable.
- Verify storage writes by reopening and hashing before deleting any prior version.
- No hardware validation has been performed.
