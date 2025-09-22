class MemorySpeakModule:
    def __init__(self, bus):
        self._bus = bus
        bus.subscribe("memory.added", self._on_added)
        bus.subscribe("memory.got", self._on_got)
        bus.subscribe("memory.forgot", self._on_forgot)

    def _on_added(self, key=None):
        self._bus.broadcast("audio.speak", text=f"Got it. I'll remember {key}.")

    def _on_got(self, key=None, result=None):
        if result is None:
            self._bus.broadcast("audio.speak", text=f"I don't have a memory for {key}.")
        else:
            self._bus.broadcast("audio.speak", text=f"{key} is {result}.")

    def _on_forgot(self, key=None):
        self._bus.broadcast("audio.speak", text=f"I've forgotten {key}.")
