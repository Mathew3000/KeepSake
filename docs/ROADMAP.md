# Implementation status

| Feature | Status |
| --- | --- |
| Immutable content-addressed server publisher | Implemented (PC-testable) |
| Atomic latest pointer | Implemented (PC-testable) |
| Configurable TLS port (server example) | Documented |
| Per-device mTLS | NGINX template only; device provisioning pending |
| ESP32 HTTPS downloader + manifest validation | Pending integration |
| ESP32 microSD version rotation (20 unique versions) | Pending integration |
| Protected manual reference snapshot | Pending |
| Heartbeat / device monitoring | Pending |
| Certificate provisioning, rotation and revocation | Pending |
| Read-only USB mass storage with USB-host detection | Pending hardware verification |
| Signed OTA and rollback | Pending |
| Power-loss / restore tests | Pending |

Treat this as a prototype, not a production backup solution. No unattended access to KeePass data is safe without careful threat modeling, encrypted payload handling and tested recovery.
