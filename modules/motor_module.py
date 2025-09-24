import time
import threading
import os

try:
    import RPi.GPIO as GPIO
    _HAS_GPIO = True
except Exception:
    _HAS_GPIO = False


class MotorModule:

    def __init__(self, bus):
        self._bus = bus
        self._simulate = os.getenv("MOTOR_SIMULATE", "0") != "0" or not _HAS_GPIO
        self._lock = threading.Lock()
        self._stopped = False

        # Example GPIO pin layout (adjust to your hardware)
        self._left_pwm_pin = 18
        self._right_pwm_pin = 19
        self._in1 = 23
        self._in2 = 24
        self._in3 = 25
        self._in4 = 8

        if not self._simulate:
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(self._left_pwm_pin, GPIO.OUT)
            GPIO.setup(self._right_pwm_pin, GPIO.OUT)
            GPIO.setup(self._in1, GPIO.OUT)
            GPIO.setup(self._in2, GPIO.OUT)
            GPIO.setup(self._in3, GPIO.OUT)
            GPIO.setup(self._in4, GPIO.OUT)
            self._left_pwm = GPIO.PWM(self._left_pwm_pin, 100)
            self._right_pwm = GPIO.PWM(self._right_pwm_pin, 100)
            self._left_pwm.start(0)
            self._right_pwm.start(0)
        else:
            print("[MotorModule] Running in simulation mode")

        bus.subscribe("robot.move.forward", self._on_move_forward)
        bus.subscribe("robot.turn.left", self._on_turn_left)
        bus.subscribe("robot.turn.right", self._on_turn_right)
        bus.subscribe("robot.stop", self._on_stop)
        bus.subscribe("emergency.stop", self._on_emergency_stop)

    def _set_motor(self, left_speed, right_speed):
        """Set motor speeds from -100..100 (negative = reverse)."""
        if self._simulate:
            print(f"[Motor] L={left_speed} R={right_speed}")
            return

        # Convert to PWM duty cycle and direction pins
        def apply(motor_speed, pwm, in_forward, in_backward):
            duty = min(max(abs(motor_speed), 0), 100)
            pwm.ChangeDutyCycle(duty)
            if motor_speed >= 0:
                GPIO.output(in_forward, GPIO.HIGH)
                GPIO.output(in_backward, GPIO.LOW)
            else:
                GPIO.output(in_forward, GPIO.LOW)
                GPIO.output(in_backward, GPIO.HIGH)

        apply(left_speed, self._left_pwm, self._in1, self._in2)
        apply(right_speed, self._right_pwm, self._in3, self._in4)

    def _on_move_forward(self, duration=1):
        with self._lock:
            if self._stopped:
                return
            print(f"[MotorModule] Moving forward for {duration}s")
            self._set_motor(80, 80)
        time.sleep(duration)
        with self._lock:
            self._set_motor(0, 0)

    def _on_turn_left(self, degrees=90):
        with self._lock:
            if self._stopped:
                return
            print(f"[MotorModule] Turning left {degrees}°")
            # Simple time-based turn; replace with real kinematics as needed
            self._set_motor(-60, 60)
        # crude mapping: 90° ~= 0.6s — adjust for your robot
        time.sleep(0.6 * (degrees / 90.0))
        with self._lock:
            self._set_motor(0, 0)

    def _on_turn_right(self, degrees=90):
        with self._lock:
            if self._stopped:
                return
            print(f"[MotorModule] Turning right {degrees}°")
            self._set_motor(60, -60)
        time.sleep(0.6 * (degrees / 90.0))
        with self._lock:
            self._set_motor(0, 0)

    def _on_stop(self, _=None):
        with self._lock:
            print("[MotorModule] Stop requested")
            self._set_motor(0, 0)

    def _on_emergency_stop(self, _=None):
        with self._lock:
            print("[MotorModule] EMERGENCY STOP")
            self._stopped = True
            self._set_motor(0, 0)
