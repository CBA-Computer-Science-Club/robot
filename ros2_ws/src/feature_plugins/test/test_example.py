import unittest
from feature_plugins.example import HeartbeatPlugin


class ExampleTests(unittest.TestCase):
    def test_plugin_uses_only_scoped_timer_and_text_publisher(self):
        class API:
            def every(self, seconds, callback):
                self.seconds = seconds
                self.callback = callback
            def publish_text(self, text):
                self.text = text
        api = API()
        HeartbeatPlugin().start(api)
        self.assertEqual(api.seconds, 5.0)
        api.callback()
        self.assertEqual(api.text, "heartbeat")


if __name__ == "__main__":
    unittest.main()
