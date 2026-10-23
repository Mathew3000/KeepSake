# Phase 1 integration status (2026-10-23)

## Added in feature/phase1-backup-client
- ESP-IDF Wi-Fi STA, NTP, mTLS GET /backup/latest.json and hash-addressed bundles
- Manifest schema/filename/size validation, checked Content-Length, streamed download, fsync, SHA-256 readback
- SD FAT history rotation limited to 20 distinct content hashes; no overwrite of existing snapshots
- mTLS HTTP POST heartbeat, localhost-only Python receiver with SQLite
- Offline CA and individual device certificates; server identity derived from the TLS client certificate CN
- Health-check CLI (nonzero on stale or missing success), pinned offline reference tool
- Unit tests for publisher, reference pinning, heartbeat and health check; GitHub Actions server test workflow

## Outstanding before calling Phase 1 complete
- **Firmware compilation with selected PlatformIO / ESP-IDF version has not been run.**
- **ESP32 SD pins and board-specific wiring are unverified.**
- Client certificates and firmware settings must be provisioned locally. Do not commit any secret.
- Client cert revocation and certificate rotation require an operational CRL/allowlist procedure; issuance alone is not device management.
- Reference pinning is currently a *server-side manual* tool, not a separately protected immutable SD partition.
- Server manifest is TLS-authenticated (mTLS), not independently digitally signed.
- Health checker exits with failure for integration with cron/monitoring; notification delivery must be configured independently.
- Fault-injection tests for interruption, FAT corruption, exhausted SD, malicious manifest and power loss are not yet automated.
- USB emergency mode, signed OTA, full device security hardening and end-to-end restore are later hardware/security phases.

**Do not treat this branch as an emergency recovery system yet.** The project is best evaluated using test credentials and synthetic data until verified on real hardware.

## Manual next steps
1. Configure `firmware/main/settings.h` from `settings.example.h`.
2. Supply three PEM files under `firmware/main/certs/`: `server-ca.pem`, `device.crt`, `device.key`.
3. Verify server NGINX config on the target host using `nginx -t`; expose only mTLS endpoint.
4. Run `python3 -m unittest discover -s tests -v`.
5. Run `pio run -d firmware` on your development workstation and report compiler output.
6. On hardware, exercise successful and failed downloads, then physically inspect card and recovery.
