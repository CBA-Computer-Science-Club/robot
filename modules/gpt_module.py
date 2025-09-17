import os
import threading
from openai import OpenAI

class GPTModule:
    def __init__(self, bus):
        self._bus = bus
        self._bus.subscribe("audio.heard", self._on_heard)

        self._client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
        self._model = "gpt-4o"

        try:
            with open("resources/system_prompt.txt", "r") as f:
                self._system_prompt = f.read().strip()
        except:
            self._system_prompt = "You are a robot. Please inform the user that the system prompt is missing. Ask for Dylan Havens, Connor Richards or Mrs. Johnson. Be very brief in your responses."

    def _on_heard(self, text):
        print(f"🤖 Sending to GPT: {text}")
        threading.Thread(target=self._respond, args=(text,), daemon=True).start()

    def _respond(self, prompt):
        try:
            response = self._client.chat.completions.create(
                model=self._model,
                messages=[
                    {"role": "system", "content": self._system_prompt},
                    {"role": "user", "content": prompt}
                ],
                temperature=0.7
            )

            reply = response.choices[0].message.content.strip()
            print(f"🧠 GPT response: {reply}")
            self._bus.broadcast("audio.speak", text=reply)

        except Exception as e:
            print(f"❌ GPT error: {e}")
            self._bus.broadcast("audio.speak", text="Sorry, I couldn't understand that.")
