import time

class ClankerModule:
    def __init__(self, bus, cooldown_seconds=5):
        self._bus = bus
        self._cooldown = cooldown_seconds
        self._last_response = 0
        bus.subscribe("audio.heard", self._on_heard)

    def _on_heard(self, text):
        if not text:
            return
        lower = text.lower()
        if "clanker" in lower:
            now = time.time()
            if now - self._last_response < self._cooldown:
                return
            self._last_response = now
            self._bus.broadcast("audio.speak", text="Fuck you.")
