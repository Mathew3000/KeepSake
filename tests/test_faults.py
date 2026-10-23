import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"server"))
from publish import publish

class FaultTests(unittest.TestCase):
    def test_rejects_missing_file_without_disturbing_pointer(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t);src=p/"input";out=p/"pub"
            src.write_bytes(b"ciphertext valid")
            first=publish(src,out)
            src.unlink()
            with self.assertRaises(ValueError):publish(src,out)
            self.assertEqual(json.loads((out/"latest.json").read_text())["sha256"],first["sha256"])
            self.assertEqual((out/first["filename"]).read_bytes(),b"ciphertext valid")
    def test_corrupt_existing_blob_never_overwritten(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t);src=p/"input";out=p/"pub"
            src.write_bytes(b"one")
            first=publish(src,out)
            (out/first["filename"]).write_bytes(b"corrupt")
            with self.assertRaises(ValueError): publish(src,out)
            self.assertEqual((out/first["filename"]).read_bytes(),b"corrupt")
    def test_stale_temporary_file_does_not_break_new_publish(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t);src=p/"input";out=p/"pub";out.mkdir()
            (out/".incoming-deadbeef").write_bytes(b"partial")
            src.write_bytes(b"valid")
            result=publish(src,out)
            self.assertEqual((out/result["filename"]).read_bytes(),b"valid")
            self.assertTrue((out/".incoming-deadbeef").exists())
if __name__=="__main__":unittest.main()
