#!/usr/bin/env python3
"""Generate NGINX mTLS fingerprint allowlist from active client certificates.

Only *.crt inside --active-dir are authorized. To revoke a device, remove its
certificate, regenerate, run nginx -t and reload NGINX. Certificates still must
pass mTLS CA validation.
"""
import argparse
import re
import subprocess
from pathlib import Path

HEX=re.compile(r"^[A-F0-9]{40}$")
def fingerprint(cert):
    p=subprocess.run(["openssl","x509","-in",str(cert),"-noout",
                      "-fingerprint","-sha1"],capture_output=True,text=True,check=True)
    fp=p.stdout.strip().split("=",1)[-1].replace(":","").upper()
    if not HEX.fullmatch(fp):raise ValueError(f"invalid certificate {cert}")
    return fp

def generate(directory):
    lines=["# Generated; include in NGINX http {} context",
           "map $ssl_client_fingerprint $keepsake_allowed {","    default 0;"]
    for cert in sorted(directory.glob("*.crt")):
        lines.append(f"    {fingerprint(cert)} 1; # {cert.stem}")
    return "\n".join(lines+["}",""])

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--active-dir",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    a=p.parse_args()
    a.output.write_text(generate(a.active_dir))
