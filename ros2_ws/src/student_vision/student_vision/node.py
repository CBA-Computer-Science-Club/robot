"""Camera-only identity source. No subscription to identity or enrollment topics."""
from pathlib import Path

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String

from .identity import TemplateStore
from .pipeline import VisionPipeline, identity_payload
from .recognizer import FaceRecognizer


class VisionNode(Node):
    def __init__(self):
        super().__init__("student_vision")
        enabled = self.declare_parameter("enabled", False).value
        if not isinstance(enabled, bool):
            raise ValueError("enabled must be a boolean")
        if not enabled:
            self.get_logger().info("Camera face matching disabled; no camera subscription or identity publication")
            return
        camera_topic = self.declare_parameter("camera_topic", "/camera/image_raw").value
        data_dir = self.declare_parameter("data_dir", str(Path.home() / ".local/share/robot/student_vision")).value
        distance = float(self.declare_parameter("maximum_distance", 60.0).value)
        margin = float(self.declare_parameter("minimum_margin", 15.0).value)
        self.store = TemplateStore(Path(data_dir))
        self._enrolled = self.store.list_people()
        self.pipeline = VisionPipeline(True, backend_factory=lambda: FaceRecognizer(distance, margin), enrolled=self._enrolled)
        self.publisher = self.create_publisher(String, "people/identity", 10)
        self.subscription = self.create_subscription(Image, camera_topic, self._on_image, 1)
        self.get_logger().info("Opt-in camera matching active; enrollment is local CLI only")

    def _on_image(self, message):
        # Re-read authorization on every frame so deletion revokes at the next callback.
        try:
            enrolled = self.store.list_people()
            if enrolled != self._enrolled:
                self.pipeline.backend.load(enrolled)
                self._enrolled = enrolled
            results = self.pipeline.process(message)
        except (ValueError, RuntimeError, OSError) as exc:
            self.get_logger().warning(f"Frame rejected: {exc}")
            results = []
        self.publisher.publish(String(data=identity_payload(results)))


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = VisionNode()
        rclpy.spin(node)
    finally:
        if node is not None:
            node.destroy_node()
        rclpy.shutdown()
