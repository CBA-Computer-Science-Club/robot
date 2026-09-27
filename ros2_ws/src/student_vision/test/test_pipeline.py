import unittest
import json
from types import SimpleNamespace
from student_vision.pipeline import VisionPipeline, identity_payload


class PipelineTests(unittest.TestCase):
    def test_disabled_does_not_create_backend_or_process_camera(self):
        def unavailable():
            raise AssertionError("backend should not initialize")
        pipe = VisionPipeline(False, backend_factory=unavailable)
        self.assertEqual(pipe.process(SimpleNamespace()), [])

    def test_enabled_only_emits_enrolled_recognizer_results(self):
        class Backend:
            def load(self, people):
                self.people = people

            def recognize(self, image):
                return [((1, 2, 3, 4), "alice"), ((5, 6, 7, 8), None)]
        backend = Backend()
        pipe = VisionPipeline(True, backend_factory=lambda: backend, enrolled={"alice": b"png"})
        self.assertEqual(backend.people, {"alice": b"png"})
        image = SimpleNamespace(width=1, height=1, step=3, encoding="bgr8", data=b"\x00\x00\x00")
        self.assertEqual(pipe.process(image), [((1, 2, 3, 4), "alice"), ((5, 6, 7, 8), None)])

    def test_only_one_face_can_supply_identity_and_empty_frame_clears(self):
        self.assertEqual(json.loads(identity_payload([]))["person_id"], None)
        self.assertEqual(json.loads(identity_payload([((1, 2, 3, 4), "alice"), ((5, 6, 7, 8), None)]))["person_id"], None)
        self.assertEqual(json.loads(identity_payload([((1, 2, 3, 4), "alice")]))["person_id"], "alice")


if __name__ == "__main__":
    unittest.main()
