import os
import tempfile
import threading
import queue
from playsound import playsound
from openai import OpenAI

class SpeechModule:
    def __init__(self, bus):
        self._queue = queue.Queue()
        self._client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
        self._model = "tts-1"
        self._voice = "onyx"  # alloy, echo, fable, onyx, nova, shimmer

        bus.subscribe("audio.speak", self._on_speak)
        threading.Thread(target=self._loop, daemon=True).start()

    def _on_speak(self, text):
        self._queue.put(text)

    def _loop(self):
        while True:
            message = self._queue.get()
            try:
                response = self._client.audio.speech.create(
                    model=self._model,
                    voice=self._voice,
                    input=message
                )

                with tempfile.NamedTemporaryFile(delete=False, suffix=".mp3") as fp:
                    fp.write(response.content)
                    temp_path = fp.name

                playsound(temp_path)

                os.remove(temp_path)

            except Exception as e:
                print("❌ TTS error:", e)
