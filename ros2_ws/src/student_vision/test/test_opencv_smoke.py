import unittest

try:
    import cv2
    import numpy as np
except ImportError:
    cv2 = np = None

from student_vision.recognizer import FaceRecognizer, choose_identity


@unittest.skipIf(cv2 is None or not hasattr(cv2, "face"), "OpenCV-contrib and NumPy not installed")
class OpenCVSmokeTests(unittest.TestCase):
    def test_contrib_lbph_models_train_on_local_png_template(self):
        recognizer = FaceRecognizer()
        template = np.arange(96 * 96, dtype=np.uint8).reshape((96, 96))
        ok, png = cv2.imencode(".png", template)
        self.assertTrue(ok)
        recognizer.load({"alice": png.tobytes()})
        self.assertEqual(list(recognizer.models), ["alice"])
        distance = float(recognizer.models["alice"].predict(template)[1])
        self.assertEqual(choose_identity({"alice": distance}, 60, 15), "alice")


if __name__ == "__main__":
    unittest.main()
