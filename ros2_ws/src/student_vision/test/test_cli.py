import unittest
from student_vision.cli import build_parser


class CLITests(unittest.TestCase):
    def test_enrollment_requires_explicit_consent_flag(self):
        parser = build_parser()
        args = parser.parse_args(["enroll", "--person-id", "alice", "--image", "face.png"])
        self.assertFalse(args.consent)
        args = parser.parse_args(["enroll", "--person-id", "alice", "--image", "face.png", "--consent"])
        self.assertTrue(args.consent)


if __name__ == "__main__":
    unittest.main()
