import os
import importlib
import inspect

def load_modules_from(folder, bus):
    for filename in os.listdir(folder):
        if filename.endswith(".py") and not filename.startswith("__"):
            modulename = filename[:-3]

            try:
                module = importlib.import_module(f"{folder}.{modulename}")

                loaded_any = False
                for name, obj in inspect.getmembers(module, inspect.isclass):
                    if obj.__module__ != module.__name__:
                        continue

                    cls = obj

                    if name.endswith("Module") or not loaded_any:
                        try:
                            cls(bus)
                            print(f"✅ Loaded {folder}/{cls.__name__}")
                            loaded_any = True
                        except TypeError:
                            try:
                                cls()
                                print(f"✅ Loaded {folder}/{cls.__name__} (no-arg)")
                                loaded_any = True
                            except Exception as e:
                                print(f"❌ Failed to instantiate {cls.__name__}: {e}")

                if not loaded_any:
                    print(f"⚠️ No suitable class found in {modulename}.py")

            except Exception as e:
                print(f"❌ Failed to load {modulename}.py: {e}")
