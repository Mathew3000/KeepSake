import hashlib
import json
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "server"))
from publish import publish

class PublishTests(unittest.TestCase):
    def test_immutable_snapshots_and_pointer(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            src, out = root / "source", root / "out"
            src.write_bytes(b"encrypted v1")
            a = publish(src, out)
            self.assertEqual(a["sha256"], hashlib.sha256(b"encrypted v1").hexdigest())
            self.assertEqual((out / a["filename"]).read_bytes(), b"encrypted v1")
            src.write_bytes(b"encrypted v2")
            b = publish(src, out)
            self.assertNotEqual(a["sha256"], b["sha256"])
            self.assertEqual((out / a["filename"]).read_bytes(), b"encrypted v1")
            self.assertEqual(json.loads((out / "latest.json").read_text())["sha256"], b["sha256"])

    def test_identical_content_does_not_duplicate_blobs(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            src, out = root / "source", root / "out"
            src.write_bytes(b"encrypted")
            a = publish(src, out)
            b = publish(src, out)
            self.assertEqual(a["filename"], b["filename"])
            self.assertEqual(len(list(out.glob("*.bundle"))), 1)

    def test_empty_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            src, out = root / "source", root / "out"
            src.write_bytes(b"")
            with self.assertRaises(ValueError): publish(src, out)

if __name__ == "__main__":
    unittest.main()
