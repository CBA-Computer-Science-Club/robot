import os
import threading
import time
import json
from datetime import datetime, timedelta
from openai import OpenAI
from tools import TOOLS

class GPTModule:
    def __init__(self, bus):
        self._bus = bus
        self._client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
        self._model = "gpt-4o"
        self._conversation = []
        self._last_active = None

        self._handlers = {
            "move_forward": self._handle_move_forward,
            "turn_left": self._handle_turn_left,
            "turn_right": self._handle_turn_right,
            "memory_add": self._handle_memory_add,
            "memory_get": self._handle_memory_get,
            "memory_search": self._handle_memory_search,
            "memory_forget": self._handle_memory_forget,
            "reset_conversation": self._handle_reset_conversation,
        }

        try:
            with open("resources/system_prompt.txt", "r") as f:
                self._system_prompt = f.read().strip()
        except:
            self._system_prompt = "System prompt is missing. Ask for help."

        bus.subscribe("audio.heard", self._on_message)

    def _on_message(self, text):
        print(f"🤖 Received message: {text}")
        threading.Thread(target=self._handle_message, args=(text,), daemon=True).start()

    def _handle_message(self, text):
        now = datetime.now()
        if not self._last_active or (now - self._last_active > timedelta(minutes=5)):
            print("🔄 Resetting conversation context (idle > 5 min)")
            self._conversation.clear()
        self._last_active = now

        self._conversation.append({"role": "user", "content": text})

        response = self._client.chat.completions.create(
            model=self._model,
            messages=[
                {"role": "system", "content": self._system_prompt},
                *self._conversation
            ],
            tools=TOOLS,
            tool_choice="auto"
        )

        message = response.choices[0].message
        tool_calls = message.tool_calls

        if tool_calls:
            for call in tool_calls:
                func_name = call.function.name
                args = json.loads(call.function.arguments)
                print(f"📞 GPT called {func_name}({args})")
                if func_name in self._handlers:
                    try:
                        result = self._handlers[func_name](**args)
                        if result is not None:
                            self._conversation.append({"role": "assistant", "content": str(result)})
                    except Exception as e:
                        print("Tool handler error:", e)

            self._conversation.append({"role": "assistant", "content": f"Executed {tool_calls[0].function.name}"})
        else:
            reply = message.content.strip()
            print(f"💬 GPT: {reply}")
            self._conversation.append({"role": "assistant", "content": reply})
            self._bus.broadcast("audio.speak", text=reply)

    def _handle_move_forward(self, duration):
        print(f"🛞 Moving forward for {duration} seconds")
        self._bus.broadcast("robot.move.forward", duration=duration)

    def _handle_memory_add(self, key, value):
        self._bus.broadcast("memory.add", key=key, value=value)
        return f"Stored memory under {key}."

    def _handle_memory_get(self, key):
        result_container = {}

        def cb(result=None):
            result_container["result"] = result

        self._bus.broadcast("memory.get", key=key, callback=cb)

        timeout = 2.0
        waited = 0.0
        interval = 0.05
        while waited < timeout and "result" not in result_container:
            time.sleep(interval)
            waited += interval

        return result_container.get("result")

    def _handle_memory_search(self, query):
        results_container = {}

        def cb(results=None):
            results_container["results"] = results

        self._bus.broadcast("memory.search", query=query, callback=cb)

        timeout = 3.0
        waited = 0.0
        interval = 0.05
        while waited < timeout and "results" not in results_container:
            time.sleep(interval)
            waited += interval

        return results_container.get("results", [])

    def _handle_memory_forget(self, key):
        self._bus.broadcast("memory.forget", key=key)
        return f"Forgot memory {key}."

    def _handle_turn_left(self, degrees):
        print(f"↪️ Turning left {degrees}°")
        self._bus.broadcast("robot.turn.left", degrees=degrees)
        
    def _handle_turn_right(self, degrees):
        print(f"↩️ Turning right {degrees}°")
        self._bus.broadcast("robot.turn.right", degrees=degrees)
    
    def _handle_reset_conversation(self):
        print("🔄 Resetting conversation context (manual)")
        self._conversation.clear()
        self._last_active = "Multiple Hours"
        self._bus.broadcast("audio.speak", text="Conversation context has been reset.")
