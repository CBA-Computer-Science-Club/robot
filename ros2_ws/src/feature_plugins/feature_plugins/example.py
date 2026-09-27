"""Example feature; inactive until explicitly approved by node parameter."""


class HeartbeatPlugin:
    def start(self, api):
        api.every(5.0, lambda: api.publish_text("heartbeat"))
