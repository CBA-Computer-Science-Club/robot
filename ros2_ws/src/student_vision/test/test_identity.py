import tempfile
import unittest
from pathlib import Path

PNG = b"\x89PNG\r\n\x1a\n" + b"example-template"

from student_vision.identity import TemplateStore


class TemplateStoreTests(unittest.TestCase):
    def test_store_starts_empty_and_rejects_enrollment_without_consent(self):
        with tempfile.TemporaryDirectory() as directory:
            store = TemplateStore(Path(directory))
            self.assertEqual(store.list_people(), {})
            with self.assertRaises(ValueError):
                store.enroll("alice", b"image", consent=False)
            self.assertEqual(store.list_people(), {})

    def test_enrollment_persists_locally_and_deletion_removes_identity_and_template(self):
        with tempfile.TemporaryDirectory() as directory:
            store = TemplateStore(Path(directory))
            store.enroll("alice", PNG, consent=True)
            self.assertEqual(TemplateStore(Path(directory)).list_people(), {"alice": PNG})
            self.assertTrue(store.delete("alice"))
            self.assertEqual(store.list_people(), {})
            self.assertEqual(list(Path(directory).iterdir()), [])
            self.assertFalse(store.delete("alice"))
    def test_invalid_ids_and_missing_consent_marker_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            store = TemplateStore(Path(directory))
            for person_id in ("../alice", "_hidden", "a/b", ""):
                with self.subTest(person_id=person_id), self.assertRaises(ValueError):
                    store.enroll(person_id, PNG, consent=True)
            store.enroll("alice", PNG, consent=True)
            (Path(directory) / "alice.json").write_text('{"person_id":"alice","consent":false}')
            self.assertEqual(store.list_people(), {})


if __name__ == "__main__":
    unittest.main()
