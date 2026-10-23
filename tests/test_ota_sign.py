import base64
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"server"))
from ota_sign import sign, manifest_bytes

class SignedOtaTests(unittest.TestCase):
    def test_signed_manifest_and_tamper_detection(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            priv,pub=d/"key.pem",d/"pub.pem"
            subprocess.run(["openssl","genpkey","-algorithm","RSA","-pkeyopt",
                            "rsa_keygen_bits:2048","-out",str(priv)],check=True,capture_output=True)
            subprocess.run(["openssl","pkey","-in",str(priv),"-pubout","-out",str(pub)],check=True,capture_output=True)
            binary=d/"firmware.bin";binary.write_bytes(b"firmware test image")
            manifest=d/"update.json"
            original=sign(binary,priv,1,"esp32-s3-usb-otg",manifest)
            signature=base64.b64decode(original["signature"])
            with tempfile.NamedTemporaryFile() as sig:
                sig.write(signature);sig.flush()
                plain={k:v for k,v in original.items() if k!="signature"}
                def verify(payload):
                    return subprocess.run(["openssl","dgst","-sha256","-verify",str(pub),
                         "-sigopt","rsa_padding_mode:pss","-sigopt","rsa_pss_saltlen:-1",
                         "-signature",sig.name],input=manifest_bytes(payload),capture_output=True).returncode
                self.assertEqual(verify(plain),0)
                plain["version"]=2
                self.assertNotEqual(verify(plain),0)
                self.assertEqual(original["sha256"],hashlib.sha256(binary.read_bytes()).hexdigest())
