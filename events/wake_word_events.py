import pvporcupine
import pyaudio
import struct
import threading
import platform
import os


class WakeWordEvents:
    def __init__(self, bus):
        self._bus = bus
        system = platform.system().lower()
        repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        resources_dir = repo_root

        if "windows" in system:
            keyword_file = os.path.join(resources_dir, "Robot_en_windows_v3_0_0.ppn")
            print("Windows OS detected")
        elif "linux" in system:
            is_rpi = False
            try:
                machine = platform.machine().lower()
                if machine.startswith("arm") or "aarch" in machine:
                    is_rpi = True
                elif os.path.exists("/proc/device-tree/model"):
                    with open("/proc/device-tree/model", "r") as f:
                        model = f.read().lower()
                        if "raspberry" in model:
                            is_rpi = True
            except Exception:
                is_rpi = False

            if is_rpi:
                keyword_file = os.path.join(resources_dir, "Robot_en_raspberry-pi_v3_0_0.ppn")
                print("Raspberry Pi detected")
            else:
                keyword_file = os.path.join(resources_dir, "Robot_en_linux_v3_0_0.ppn")
                print("Linux OS detected")
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
