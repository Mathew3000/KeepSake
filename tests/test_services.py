import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path
from datetime import datetime, timezone

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"server"))
from heartbeat import update
from check_health import check
from reference import pin, sha256

class BackupServiceTests(unittest.TestCase):
    def test_heartbeat_tracking(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/"heartbeat.db"
            digest="a"*64
            with sqlite3.connect(p) as db:
                update(db,"offsite-01",digest,"success")
                update(db,"offsite-01",digest,"failure")
                row=db.execute("SELECT last_success,last_sha,result FROM heartbeats").fetchone()
                self.assertIsNotNone(row[0])
                self.assertEqual(row[1],digest)
                self.assertEqual(row[2],"failure")
            self.assertEqual(check(p,["offsite-01"],48),[])
            self.assertTrue(check(p,["absent"],48))

    def test_device_identity_validation(self):
        with sqlite3.connect(":memory:") as db:
            with self.assertRaises(ValueError):update(db,"../evil","a"*64,"success")
            with self.assertRaises(ValueError):update(db,"valid","notahash","success")

    def test_reference_is_immutable(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t);src=p/"encrypted.bundle";src.write_bytes(b"ciphertext")
            first=pin(src,p/"references")
            self.assertEqual(sha256(first),sha256(src))
            self.assertEqual(pin(src,p/"references"),first)
            src.write_bytes(b"another ciphertext")
            second=pin(src,p/"references")
            self.assertNotEqual(first,second)
            self.assertTrue(first.exists())

if __name__=="__main__":unittest.main()
