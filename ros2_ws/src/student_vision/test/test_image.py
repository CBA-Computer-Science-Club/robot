import unittest
from types import SimpleNamespace

from student_vision.image import decode_image


class DecodeImageTests(unittest.TestCase):
    def test_rgb8_with_row_padding_and_bgr8_conversion(self):
        rgb = SimpleNamespace(width=2, height=1, step=8, encoding="rgb8", data=bytes([1, 2, 3, 4, 5, 6, 0, 0]))
        bgr = SimpleNamespace(width=1, height=1, step=3, encoding="bgr8", data=bytes([3, 2, 1]))
        self.assertEqual(decode_image(rgb).bgr, bytes([3, 2, 1, 6, 5, 4]))
        self.assertEqual(decode_image(bgr).bgr, bytes([3, 2, 1]))

    def test_rejects_truncated_or_unsupported_frame(self):
        for encoding, step, data in [("jpeg", 3, b"abc"), ("rgb8", 3, b"ab")]:
            with self.subTest(encoding=encoding, data=data), self.assertRaises(ValueError):
                decode_image(SimpleNamespace(width=1, height=1, step=step, encoding=encoding, data=data))


if __name__ == "__main__":
    unittest.main()
