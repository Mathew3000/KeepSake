<p align="center">
  <img src="assets/keepsake-logo.svg" alt="KeepSake logo" width="180">
</p>

# KeepSake

An ESP32-S3 based offsite emergency backup appliance for encrypted KeePass databases and digital-legacy documentation.

**Status: early development prototype. Do not use as your only backup.**

## Design

- Per-device mTLS identity; custom HTTPS port supported.
- Server publishes immutable, SHA-256-addressed backup bundles and atomically updates a pointer.
- Client retains 20 verified, distinct backups (only changed content creates a version).
- A separate manual reference backup must be kept outside automatic rotation.
- Planned: SD read-only USB emergency mode, heartbeat monitoring, credential provisioning, signed OTA.

## Repository

- `firmware/`: PlatformIO / ESP-IDF firmware development skeleton.
- `server/`: publishing endpoint and NGINX mTLS template.
- `tests/`: server-side automated tests.
- `docs/`: recovery and security guidance.

## Development status (important)

The feature branch now includes the mTLS backup client, 20-version history,
a heartbeat receiver, certificate issuance, reference pinning, an offline
German recovery page, and a *tested* host-independent USB ownership policy.

**Not yet operating on a USB-connected PC:** the actual write-protected
TinyUSB MSC block device and USB host enumeration handoff. The experimental
probe is in `firmware/experimental/` but is intentionally excluded from the
firmware build until the driver can enforce write protection. Hardware
validation is essential.

OTA manifest signing works server-side, but ESP-side signature verification,
firmware installation, and rollback are still pending. Device certificate
revocation and physical recovery tests are also pending.

See [payload and recovery format](docs/PAYLOAD_FORMAT.md), [USB emergency integration](docs/USB_EMERGENCY.md) and
[roadmap](docs/ROADMAP.md) for an exact list.

## Getting started

Run `python3 -m unittest discover -s tests -v` to test the publisher.
Run `python3 server/publish.py --input /path/to/encrypted-backup.bundle --output /srv/keepsake` to publish a snapshot.
In NGINX configure HTTPS, a private CA for client certificates, and a listen port of your choosing.

**Never commit**: KeePass databases, plaintext emergency documents, credentials, keys, or personal infrastructure information.

## Hardware caveat

Exact SD wiring, USB port routing, USB enumeration, power-loss behavior and ESP-IDF integration require validation on the ESP32-S3-USB-OTG board. USB storage is not implemented in this initial checked-in firmware.
