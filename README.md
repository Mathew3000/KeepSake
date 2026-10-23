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

## Getting started

Run `python3 -m unittest discover -s tests -v` to test the publisher.
Run `python3 server/publish.py --input /path/to/encrypted-backup.bundle --output /srv/keepsake` to publish a snapshot.
In NGINX configure HTTPS, a private CA for client certificates, and a listen port of your choosing.

**Never commit**: KeePass databases, plaintext emergency documents, credentials, keys, or personal infrastructure information.

## Hardware caveat

Exact SD wiring, USB port routing, USB enumeration, power-loss behavior and ESP-IDF integration require validation on the ESP32-S3-USB-OTG board. USB storage is not implemented in this initial checked-in firmware.
