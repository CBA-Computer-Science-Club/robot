"""Small convenience facade; trusted Python plugins are NOT sandboxed."""
import re


class ScopedAPI:
    def __init__(self, node, name, message_factory):
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", name):
            raise ValueError("Plugin name must be a safe ROS name")
        self._node = node
        self._message_factory = message_factory
        self.publisher = node.create_publisher(message_factory, f"feature_plugins/{name}", 10)
        self._timers = []

    def publish_text(self, text):
        if not isinstance(text, str) or len(text) > 4096:
            raise ValueError("Text payload must be a string of at most 4096 characters")
        message = self._message_factory()
        message.data = text
        self.publisher.publish(message)

    def every(self, seconds, callback):
        if not 0.1 <= seconds <= 3600:
            raise ValueError("Timer interval must be between 0.1 and 3600 seconds")
        if not callable(callback):
            raise ValueError("Timer callback must be callable")
        timer = self._node.create_timer(seconds, callback)
        self._timers.append(timer)
        return timer
