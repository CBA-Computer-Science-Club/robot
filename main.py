from core.event_bus import EventBus
from core.loader import load_modules_from

bus = EventBus()

load_modules_from("modules", bus)
load_modules_from("events", bus)

bus.broadcast("system.startup")

bus.dispatch_loop()
