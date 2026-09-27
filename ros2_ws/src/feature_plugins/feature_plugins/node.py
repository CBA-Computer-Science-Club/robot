"""Only explicitly approved installed entry points run. Python code is not sandboxed."""
from importlib.metadata import entry_points

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

from .api import ScopedAPI
from .registry import start_plugins


class PluginNode(Node):
    def __init__(self):
        super().__init__("feature_plugins")
        approvals = self.declare_parameter("approved_plugins", []).value
        if not isinstance(approvals, (list, tuple)) or any(not isinstance(item, str) for item in approvals):
            raise ValueError("approved_plugins must be a string list")
        # No entry point is imported/loaded until it exactly matches the approval.
        candidates = entry_points(group="robot.feature_plugins") if approvals else []
        self.plugins = start_plugins(approvals, candidates, lambda name: ScopedAPI(self, name, String))
        self.get_logger().info(f"Started {len(self.plugins)} approved plugins")


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = PluginNode()
        rclpy.spin(node)
    finally:
        if node is not None:
            for plugin in reversed(node.plugins):
                stop = getattr(plugin, "stop", None)
                if callable(stop):
                    stop()
            node.destroy_node()
        rclpy.shutdown()
