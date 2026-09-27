"""Launch structure contract; no ROS packages needed to run these checks."""
import ast
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class BringupContract(unittest.TestCase):
    def test_launch_file_is_valid_python(self):
        launch = ROOT / 'robot_bringup/launch/robot.launch.py'
        ast.parse(launch.read_text())

    def test_navigation_remains_separate_from_motor_arming(self):
        source = (ROOT / 'robot_bringup/launch/robot.launch.py').read_text()
        self.assertIn('armed', source)
        self.assertIn('False', source)
        self.assertIn('slam_toolbox', source)
        self.assertIn('nav2_bringup', source)
        self.assertIn('motor_controller', source)


if __name__ == '__main__':
    unittest.main()
