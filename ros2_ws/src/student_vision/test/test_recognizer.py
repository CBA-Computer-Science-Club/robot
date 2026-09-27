import unittest
from student_vision.recognizer import choose_identity


class MatchingTests(unittest.TestCase):
    def test_unknown_by_default_and_ambiguous_or_weak_match(self):
        self.assertIsNone(choose_identity({}, 60, 15))
        self.assertIsNone(choose_identity({"alice": 61}, 60, 15))
        self.assertIsNone(choose_identity({"alice": 40, "bob": 50}, 60, 15))
        self.assertEqual(choose_identity({"alice": 40, "bob": 65}, 60, 15), "alice")


if __name__ == "__main__":
    unittest.main()
