import importlib
import sys
import types
import unittest
from unittest.mock import patch


class FakeNode:
    def __init__(self, name):
        self.name = name
        self.params = []

    def declare_parameter(self, name, default):
        self.params.append((name, default))
        return types.SimpleNamespace(value=default)

    def get_logger(self):
        return types.SimpleNamespace(info=lambda text: None)


class PluginNodeTests(unittest.TestCase):
    def test_default_approvals_load_no_plugins(self):
        node_mod = types.ModuleType("rclpy.node")
        node_mod.Node = FakeNode
        msg_mod = types.ModuleType("std_msgs.msg")
        msg_mod.String = type("String", (), {})
        with patch.dict(sys.modules, {"rclpy": types.ModuleType("rclpy"), "rclpy.node": node_mod,
                                      "std_msgs": types.ModuleType("std_msgs"), "std_msgs.msg": msg_mod}):
            module = importlib.import_module("feature_plugins.node")
            node = module.PluginNode()
            self.assertEqual(node.plugins, [])
            self.assertIn(("approved_plugins", []), node.params)


if __name__ == "__main__":
    unittest.main()
