import os
import tempfile
import threading
import queue
from playsound3 import playsound
from openai import OpenAI

class SpeechModule:
    def __init__(self, bus):
        self._queue = queue.Queue()
        self._client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
        self._model = "tts-1"
        self._voice = "onyx"  # alloy, echo, fable, onyx, nova, shimmer
        self._current_sound = None
        self._lock = threading.Lock()

        bus.subscribe("audio.speak", self._on_speak)
        bus.subscribe("audio.speak.stop", self._on_stop)
        threading.Thread(target=self._loop, daemon=True).start()

    def _on_speak(self, text):
        self._queue.put(text)

    def _on_stop(self, _=None):
        with self._lock:
            if self._current_sound and self._current_sound.is_alive():
                self._current_sound.stop()
                self._current_sound = None

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

                with self._lock:
                    self._current_sound = playsound(temp_path, block=False)
                self._current_sound.wait()
                with self._lock:
                    self._current_sound = None
                os.remove(temp_path)

            except Exception as e:
                print("❌ TTS error:", e)
