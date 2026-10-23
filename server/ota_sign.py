#!/usr/bin/env python3
"""Sign a firmware manifest on the server. Device verification is not yet implemented."""
import argparse
import base64
import hashlib
import json
import subprocess
from pathlib import Path

def manifest_bytes(data):
    return json.dumps(data,sort_keys=True,separators=(',',':')).encode('utf-8')

def sign(binary,key,version,board,output):
    data={'schema':1,'board':board,'version':version,
          'size':binary.stat().st_size,
          'sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
    signature=subprocess.run(
        ['openssl','dgst','-sha256','-sign',str(key),
         '-sigopt','rsa_padding_mode:pss','-sigopt','rsa_pss_saltlen:-1'],
        input=manifest_bytes(data),capture_output=True,check=True).stdout
    result={**data,'signature':base64.b64encode(signature).decode('ascii')}
    output.write_text(json.dumps(result,indent=2)+'\n')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',required=True,type=Path)
    parser.add_argument('--private-key',required=True,type=Path)
    parser.add_argument('--version',required=True,type=int)
    parser.add_argument('--board',default='esp32-s3-usb-otg')
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    sign(args.binary,args.private_key,args.version,args.board,args.output)
