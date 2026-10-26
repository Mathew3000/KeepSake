#!/usr/bin/env python3
"""Offline certificate admin for KeepSake devices. Requires OpenSSL on PATH.

Do not store the CA signing key in the web server's public directories.
One CA per deployment; clients must never receive the CA private key.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

DEVICE = re.compile(r"^[A-Za-z0-9_-]{1,48}$")

def openssl(*args):
    subprocess.run(["openssl", *map(str, args)], check=True)

def init_ca(output):
    output.mkdir(parents=True, exist_ok=True)
    key, cert = output / "ca.key", output / "ca.crt"
    if key.exists() or cert.exists():
        raise FileExistsError("CA already initialized")
    openssl("req", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
            "-nodes", "-days", "3650", "-sha256", "-keyout", key, "-out", cert,
            "-subj", "/CN=KeepSake Client CA",
            "-addext", "basicConstraints=critical,CA:TRUE",
            "-addext", "keyUsage=critical,keyCertSign,cRLSign")
    key.chmod(0o600)

def enroll(output, device):
    if not DEVICE.fullmatch(device):
        raise ValueError("bad device identifier")
    ca_key, ca_cert = output / "ca.key", output / "ca.crt"
    if not ca_key.exists() or not ca_cert.exists():
        raise ValueError("initialize CA first")
    folder = output / "devices" / device
    folder.mkdir(parents=True, exist_ok=False)
    key, csr, crt = folder / "device.key", folder / "device.csr", folder / "device.crt"
    openssl("req", "-new", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
            "-nodes", "-keyout", key, "-out", csr, "-subj", f"/CN={device}")
    key.chmod(0o600)
    ext = folder / "extensions.cnf"
    ext.write_text("basicConstraints=CA:FALSE\nkeyUsage=digitalSignature\nextendedKeyUsage=clientAuth\n")
    openssl("x509", "-req", "-in", csr, "-CA", ca_cert, "-CAkey", ca_key,
            "-CAcreateserial", "-out", crt, "-days", "365", "-sha256", "-extfile", ext)
    print(f"New certificate: {crt}; keep private key: {key}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["init-ca", "enroll"])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--device")
    args = parser.parse_args()
    if args.command == "init-ca":
        init_ca(args.output)
    elif not args.device:
        parser.error("--device is required for enrollment")
    else:
        enroll(args.output, args.device)

if __name__ == "__main__":
    main()
