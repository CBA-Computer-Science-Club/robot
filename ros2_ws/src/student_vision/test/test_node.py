import importlib
import sys
import types
import unittest
from unittest.mock import patch


class FakeNode:
    def __init__(self, name):
        self.subscriptions = []
        self.publishers = []
        self.params = {"enabled": False}

    def declare_parameter(self, name, default):
        return types.SimpleNamespace(value=self.params.get(name, default))

    def create_subscription(self, msg, topic, callback, qos):
        self.subscriptions.append(topic)

    def create_publisher(self, msg, topic, qos):
        self.publishers.append(topic)
        return types.SimpleNamespace(publish=lambda message: None)

    def get_logger(self):
        return types.SimpleNamespace(info=lambda message: None)


class VisionNodeTests(unittest.TestCase):
    def test_camera_disabled_default_creates_no_subscription_or_identity_publisher(self):
        fake_node = types.ModuleType("rclpy.node")
        fake_node.Node = FakeNode
        fake_image = types.ModuleType("sensor_msgs.msg")
        fake_image.Image = type("Image", (), {})
        fake_string = types.ModuleType("std_msgs.msg")
        fake_string.String = type("String", (), {})
        with patch.dict(sys.modules, {"rclpy": types.ModuleType("rclpy"), "rclpy.node": fake_node,
                                      "sensor_msgs": types.ModuleType("sensor_msgs"), "sensor_msgs.msg": fake_image,
                                      "std_msgs": types.ModuleType("std_msgs"), "std_msgs.msg": fake_string}):
            module = importlib.import_module("student_vision.node")
            node = module.VisionNode()
            self.assertEqual(node.subscriptions, [])
            self.assertEqual(node.publishers, [])


if __name__ == "__main__":
    unittest.main()
