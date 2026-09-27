import unittest
from feature_plugins.api import ScopedAPI


class ScopedAPITests(unittest.TestCase):
    def test_publishes_only_on_its_owned_topic_and_rejects_fast_timers(self):
        class Publisher:
            def publish(self, msg):
                self.messages.append(msg.data)
            messages = []
        class Node:
            def create_publisher(self, msg_type, topic, qos):
                self.topic = topic
                return Publisher()
            def create_timer(self, period, callback):
                self.period = period
                self.callback = callback
                return object()
        node = Node()
        api = ScopedAPI(node, "pulse", lambda: type("Msg", (), {})())
        api.publish_text("hello")
        self.assertEqual(node.topic, "feature_plugins/pulse")
        self.assertEqual(api.publisher.messages, ["hello"])
        with self.assertRaises(ValueError):
            api.every(0.01, lambda: None)
        api.every(1.0, lambda: None)
        self.assertEqual(node.period, 1.0)


if __name__ == "__main__":
    unittest.main()
