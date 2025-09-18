import pvporcupine
import pyaudio
import struct
import threading
import platform

class WakeWordEvents:
    def __init__(self, bus):
        self._bus = bus
        system = platform.system().lower()  #Detect OS for .ppn file
        if "windows" in system:
            keyword_file = "Robot_en_windows_v3_0_0.ppn"
        elif "linux" in system:                             #NEED FILES
            keyword_file = "Robot_en_linux_v3_0_0.ppn"
        elif "darwin" in system:  # macOS
            keyword_file = "Robot_en_mac_v3_0_0.ppn"
        else:
            raise RuntimeError(f"Unsupported OS: {system}")
        self._porcupine = pvporcupine.create(
            access_key="eX8HXEKznFEwoenIrdNaVXWXGsyopppQnr1t7LNXUNkEwQjWazPGUg==",
            keyword_paths=[keyword_file]
        )
        self._pa = pyaudio.PyAudio()
        threading.Thread(target=self._loop, daemon=True).start()
    
    def _loop(self):
        stream = self._pa.open(
            rate=self._porcupine.sample_rate,
            channels=1,
            format=pyaudio.paInt16,
            input=True,
            frames_per_buffer=self._porcupine.frame_length
        )

        print("🎤 Listening for custom wake word...")
        while True:
            pcm = stream.read(self._porcupine.frame_length, exception_on_overflow=False)
            pcm = struct.unpack_from("h" * self._porcupine.frame_length, pcm)

            result = self._porcupine.process(pcm)
            if result >= 0:
                print("✅ Wake word detected!")
                self._bus.broadcast("audio.wake_word")
