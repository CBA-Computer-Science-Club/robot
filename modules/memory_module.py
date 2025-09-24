import os
import json
import threading
import time


class MemoryModule:
    
    PATH = os.path.join("resources", "memory.json")

    def __init__(self, bus):
        self._bus = bus
        self._lock = threading.Lock()
        self._data = {}

        bus.subscribe("memory.add", self._on_add)
        bus.subscribe("memory.get", self._on_get)
        bus.subscribe("memory.search", self._on_search)
        bus.subscribe("memory.list", self._on_list)
        bus.subscribe("memory.forget", self._on_forget)

        try:
            if os.path.exists(self.PATH):
                with open(self.PATH, "r") as f:
                    self._data = json.load(f)
        except Exception:
            self._data = {}

    def _persist(self):
        os.makedirs(os.path.dirname(self.PATH), exist_ok=True)
        with open(self.PATH, "w") as f:
            json.dump(self._data, f, indent=2)

    def _on_add(self, key=None, value=None):
        if not key:
            return
        with self._lock:
            self._data[key] = {"value": value, "ts": time.time()}
            try:
                self._persist()
            except Exception:
                pass
        self._bus.broadcast("memory.added", key=key)

    def _on_get(self, key=None, callback=None):
        result = None
        with self._lock:
            item = self._data.get(key)
            if item:
                result = item.get("value")
        if callback and callable(callback):
            try:
                callback(result=result)
            except Exception:
                pass
        self._bus.broadcast("memory.got", key=key, result=result)

    def _on_search(self, query=None, callback=None):
        results = []
        if not query:
            results = []
        else:
            q = query.lower()
            with self._lock:
                for k, v in self._data.items():
                    if q in k.lower() or (v and q in str(v.get("value", "")).lower()):
                        results.append({"key": k, "value": v.get("value")})
        if callback and callable(callback):
            try:
                callback(results=results)
            except Exception:
                pass
        self._bus.broadcast("memory.search_results", results=results, query=query)

    def _on_list(self, _=None):
        with self._lock:
            items = [{"key": k, "value": v.get("value")} for k, v in self._data.items()]
        self._bus.broadcast("memory.listed", items=items)

    def _on_forget(self, key=None):
        with self._lock:
            if key in self._data:
                del self._data[key]
                try:
                    self._persist()
                except Exception:
                    pass
                self._bus.broadcast("memory.forgot", key=key)
