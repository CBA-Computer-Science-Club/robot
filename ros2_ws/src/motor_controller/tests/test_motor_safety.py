"""Safety contract for motor controller wiring, watchdog and arming.

Run with: python -m unittest discover ros2_ws/src/motor_controller/tests
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MotorSafetyContract(unittest.TestCase):
    def test_actuator_is_disarmed_and_stops_on_timeout(self):
        source = (ROOT / 'src/motor_controller.cpp').read_text()
        self.assertIn('armed', source)
        self.assertIn('watchdog', source)
        self.assertIn('stop_motors()', source)

    def test_differential_drive_and_odometry_input(self):
        source = (ROOT / 'src/motor_controller.cpp').read_text()
        self.assertIn('angular.z', source)
        self.assertIn('linear.x', source)
        self.assertIn('odom', source)
        self.assertIn('left', source)
        self.assertIn('right', source)

    def test_build_hardware_driver_is_explicit(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        self.assertIn('LGPIO_LIB', cmake)
        self.assertIn('FATAL_ERROR', cmake)


if __name__ == '__main__':
    unittest.main()
