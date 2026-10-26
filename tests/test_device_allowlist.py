import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"server"))
from device_allowlist import generate

class AllowlistTests(unittest.TestCase):
    def test_revoke_by_removal(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)
            subprocess.run(["openssl","req","-x509","-newkey","rsa:2048","-nodes",
                            "-keyout",str(p/"test.key"),"-out",str(p/"unit.crt"),"-days","1",
                            "-subj","/CN=unit"],check=True,capture_output=True)
            allowed=generate(p)
            self.assertIn("unit",allowed)
            self.assertIn(" 1;",allowed)
            (p/"unit.crt").unlink()
            blocked=generate(p)
            self.assertNotIn(" 1;",blocked)
            self.assertIn("default 0",blocked)
if __name__=="__main__": unittest.main()
