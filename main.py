from core.event_bus import EventBus
from core.loader import load_modules_from
import threading
import sys

bus = EventBus()

load_modules_from("modules", bus)
load_modules_from("events", bus)

bus.broadcast("system.startup")


def _terminal_input_loop(bus):
	
	try:
		while True:
			try:
				text = input("> ").strip()
			except EOFError:
				break
			if not text:
				continue
			print(f"(terminal) heard: {text}")
			bus.broadcast("audio.heard", text=text)
	except Exception as e:
		print("Terminal input loop error:", e)


threading.Thread(target=_terminal_input_loop, args=(bus,), daemon=True).start()

bus.dispatch_loop()
