#!/usr/bin/env python3
"""Publish immutable SHA256-addressed snapshots with an atomic latest pointer.

The input MUST ALREADY BE ENCRYPTED; never publish plaintext documentation.
This script is intentionally independent of Nextcloud.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
from datetime import datetime, timezone

MAX_BYTES = 64 * 1024 * 1024

def publish(source: Path, destination: Path) -> dict:
    if not source.is_file():
        raise ValueError("input must be a regular file")
    destination.mkdir(parents=True, exist_ok=True)
    # Guard against accidental multi-gigabyte downloads on a microcontroller.
    if source.stat().st_size == 0 or source.stat().st_size > MAX_BYTES:
        raise ValueError("source size invalid")
    h = hashlib.sha256()
    with tempfile.NamedTemporaryFile(dir=destination, prefix=".incoming-", delete=False) as tmp:
        temp_path = Path(tmp.name)
        try:
            with source.open("rb") as inp:
                while (data := inp.read(65536)):
                    h.update(data)
                    tmp.write(data)
            tmp.flush()
            os.fsync(tmp.fileno())
            digest = h.hexdigest()
            target = destination / f"{digest}.bundle"
            if target.exists():
                if hashlib.sha256(target.read_bytes()).hexdigest() != digest:
                    raise ValueError("existing blob hash mismatch")
            else:
                os.replace(temp_path, target)
            pointer = {
                "schema": 1, "sha256": digest, "filename": f"{digest}.bundle",
                "size": source.stat().st_size,
                "published_at": datetime.now(timezone.utc).isoformat(),
            }
            with tempfile.NamedTemporaryFile(dir=destination, mode="w", encoding="utf-8",
                                             prefix=".manifest-", delete=False) as mf:
                manifest_tmp = Path(mf.name)
                json.dump(pointer, mf, separators=(",", ":"))
                mf.write("\n")
                mf.flush()
                os.fsync(mf.fileno())
            os.replace(manifest_tmp, destination / "latest.json")
            dir_fd = os.open(destination, os.O_RDONLY)
            try: os.fsync(dir_fd)
            finally: os.close(dir_fd)
            return pointer
        finally:
            temp_path.unlink(missing_ok=True)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    args = ap.parse_args()
    print(json.dumps(publish(args.input, args.output), indent=2))
