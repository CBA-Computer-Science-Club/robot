"""ROS-independent opt-in gate and recognition pipeline."""
import json
from .image import decode_image
from .recognizer import FaceRecognizer


def identity_payload(results):
    """One face maximum; unknown clears prior identities, including ambiguous frames."""
    box, person_id = results[0] if len(results) == 1 else (None, None)
    return json.dumps({"person_id": person_id, "source": "camera", "bbox": box})


class VisionPipeline:
    def __init__(self, enabled=False, *, backend_factory=FaceRecognizer, enrolled=None):
        self.enabled = enabled
        self.backend = None
        if enabled:
            self.backend = backend_factory()
            self.backend.load(enrolled or {})

    def process(self, image_message):
        if not self.enabled:
            return []
        return self.backend.recognize(decode_image(image_message))
