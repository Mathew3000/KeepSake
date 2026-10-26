#!/usr/bin/env python3
"""Manually pin and verify a recovery snapshot outside device auto-rotation."""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path

def sha256(path):
    with path.open("rb") as f:
        h=hashlib.sha256()
        for b in iter(lambda:f.read(65536),b""):h.update(b)
        return h.hexdigest()

def pin(source, reference_dir):
    reference_dir.mkdir(parents=True, exist_ok=True)
    digest=sha256(source)
    path=reference_dir / f"reference-{digest}.bundle"
    if path.exists():
        if sha256(path)!=digest: raise ValueError("existing reference corrupted")
    else:
        # Create exclusively. Never overwrite a reference.
        with source.open("rb") as inp, path.open("xb") as out:
            shutil.copyfileobj(inp,out)
            out.flush();os.fsync(out.fileno())
        if sha256(path)!=digest: raise ValueError("reference verification failed")
    print(json.dumps({"reference":str(path),"sha256":digest}))
    return path

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("command",choices=["pin","verify"])
    ap.add_argument("file",type=Path)
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    if args.command=="pin":
        if not args.output: ap.error("--output required")
        pin(args.file,args.output)
    else:
        name=args.file.name
        digest=name[len("reference-"):-len(".bundle")] if name.startswith("reference-") and name.endswith(".bundle") else ""
        if len(digest)!=64 or sha256(args.file)!=digest:
            raise SystemExit("verification FAILED")
        print("verified",digest)
if __name__=="__main__":main()
