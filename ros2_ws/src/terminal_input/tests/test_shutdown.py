from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1] / 'src' / 'terminal_input.cpp'

class ShutdownSafety(unittest.TestCase):
    def test_reader_waits_with_timeout_rather_than_blocked_getline(self):
        code = SOURCE.read_text()
        self.assertIn('poll(', code)
        self.assertIn('read(', code)
        self.assertNotIn('std::getline', code)

if __name__ == '__main__':
    unittest.main()
