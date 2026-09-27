"""OpenCV Haar detection + LBPH template matching (not a biometric accuracy guarantee)."""
import math


def choose_identity(distances, maximum_distance, minimum_margin):
    """Fail closed for no candidate, weak score or ambiguous runners-up."""
    if maximum_distance <= 0 or minimum_margin < 0 or not distances:
        return None
    ranked = sorted(distances.items(), key=lambda item: item[1])
    name, best = ranked[0]
    if not math.isfinite(best) or best >= maximum_distance:
        return None
    if len(ranked) > 1 and (not math.isfinite(ranked[1][1]) or ranked[1][1] - best < minimum_margin):
        return None
    return name


class FaceRecognizer:
    def __init__(self, maximum_distance=60.0, minimum_margin=15.0):
        try:
            import cv2
            import numpy as np
        except ImportError as exc:
            raise RuntimeError("Requires OpenCV with contrib face module and NumPy") from exc
        if not hasattr(cv2, "face") or not hasattr(cv2.face, "LBPHFaceRecognizer_create"):
            raise RuntimeError("OpenCV contrib face/LBPH module unavailable; install a matching OpenCV-contrib build")
        if maximum_distance <= 0 or minimum_margin < 0:
            raise ValueError("Invalid LBPH threshold or margin")
        self.cv2, self.np = cv2, np
        self.maximum_distance = maximum_distance
        self.minimum_margin = minimum_margin
        cascade = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
        self.detector = cv2.CascadeClassifier(cascade)
        if self.detector.empty():
            raise RuntimeError("OpenCV frontal-face cascade unavailable")
        self.models = {}

    def _gray(self, bgr):
        return self.cv2.cvtColor(bgr, self.cv2.COLOR_BGR2GRAY)

    def _template(self, gray_face):
        resized = self.cv2.resize(gray_face, (96, 96), interpolation=self.cv2.INTER_AREA)
        return self.cv2.equalizeHist(resized)

    def _faces(self, bgr):
        gray = self._gray(bgr)
        boxes = self.detector.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(60, 60))
        return [(tuple(map(int, (x, y, w, h))), self._template(gray[y:y+h, x:x+w])) for x, y, w, h in boxes]

    def _decode_frame(self, frame):
        return self.np.frombuffer(frame.bgr, dtype=self.np.uint8).reshape((frame.height, frame.width, 3))

    def enrollment_template(self, image_bgr):
        faces = self._faces(image_bgr)
        if len(faces) != 1:
            raise ValueError("Enrollment requires exactly one detected frontal face")
        ok, encoded = self.cv2.imencode(".png", faces[0][1])
        if not ok:
            raise RuntimeError("Could not encode face template")
        return encoded.tobytes()

    def load(self, enrolled):
        models = {}
        for person_id, data in enrolled.items():
            face = self.cv2.imdecode(self.np.frombuffer(data, dtype=self.np.uint8), self.cv2.IMREAD_GRAYSCALE)
            if face is None or face.shape != (96, 96):
                continue
            model = self.cv2.face.LBPHFaceRecognizer_create()
            model.train([face], self.np.array([0], dtype=self.np.int32))
            models[person_id] = model
        self.models = models

    def recognize(self, frame):
        """Return (bbox, consented ID or None) for each detected face."""
        results = []
        for box, face in self._faces(self._decode_frame(frame)):
            distances = {person_id: float(model.predict(face)[1]) for person_id, model in self.models.items()}
            results.append((box, choose_identity(distances, self.maximum_distance, self.minimum_margin)))
        return results
