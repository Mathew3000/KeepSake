#!/usr/bin/env python3
"""Local-only heartbeat receiver; NGINX performs mTLS and forwards verified CN.

NEVER expose this service directly to the network. Use NGINX proxy_pass to
127.0.0.1:8765; overwrite identity headers and permit only verified clients.
"""
import argparse
import json
import re
import sqlite3
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

DEVICE_ID = re.compile(r"^[a-zA-Z0-9_-]{1,48}$")
HASH = re.compile(r"^[a-f0-9]{64}$")


def update(db, device, digest, result):
    if not DEVICE_ID.fullmatch(device):
        raise ValueError("invalid device")
    if digest and not HASH.fullmatch(digest):
        raise ValueError("invalid hash")
    if result not in {"success", "failure"}:
        raise ValueError("invalid result")
    db.execute("""CREATE TABLE IF NOT EXISTS heartbeats(
        device TEXT PRIMARY KEY, last_seen TEXT NOT NULL,
        last_success TEXT, last_sha TEXT, result TEXT NOT NULL)""")
    stamp = datetime.now(timezone.utc).isoformat()
    db.execute("""INSERT INTO heartbeats(device,last_seen,last_success,last_sha,result)
      VALUES(?,?,?,?,?)
      ON CONFLICT(device) DO UPDATE SET
      last_seen=excluded.last_seen,
      last_success=CASE WHEN excluded.result='success' THEN excluded.last_success
                   ELSE heartbeats.last_success END,
      last_sha=CASE WHEN excluded.result='success' THEN excluded.last_sha
                   ELSE heartbeats.last_sha END,
      result=excluded.result""",
      (device, stamp, stamp if result == "success" else None, digest, result))
    db.commit()


class Receiver(BaseHTTPRequestHandler):
    def do_POST(self):
        if urlsplit(self.path).path != "/backup/heartbeat":
            self.send_error(404); return
        # Trusted only because service binds loopback, and NGINX replaces this header.
        device = self.headers.get("X-Keepsake-Device", "")
        try:
            if not DEVICE_ID.fullmatch(device):
                raise ValueError("missing verified device CN")
            size = int(self.headers.get("Content-Length", "0"))
            if size < 2 or size > 1024:
                raise ValueError("body size invalid")
            payload = json.loads(self.rfile.read(size))
            if payload.get("device") != device:
                raise ValueError("device mismatch")
            with sqlite3.connect(self.server.db_path) as db:
                update(db, device, payload.get("sha256", ""), payload.get("result"))
        except (ValueError, KeyError, TypeError, json.JSONDecodeError):
            self.send_error(400); return
        self.send_response(204); self.end_headers()
    def log_message(self, fmt, *args):
        pass


def run(db_path, bind="127.0.0.1", port=8765):
    server = ThreadingHTTPServer((bind, port), Receiver)
    server.db_path = str(db_path)
    server.serve_forever()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--database", type=Path, default=Path("heartbeats.sqlite"))
    args = parser.parse_args()
    run(args.database)
