# Campus conversation robot — ROS 2 workspace

A C++-first ROS 2 robot skeleton for a Raspberry Pi 5 running Ubuntu 24.04 arm64 and ROS 2 Jazzy. The source tree lives in `ros2_ws/src`. This is **not a working autonomous robot until real sensors, encoders, motor drivers, safety hardware, and an inference runtime are integrated and tested on the robot**. No sample code should be taken as a safety-certified controller.

## Architecture

| Package | Responsibility |
|---|---|
| `motor_controller` | C++ differential H-bridge driver; starts disarmed, demands fresh `/scan` and `/odom`, watchdog and latched ROS E-stop |
| `robot_bringup` | Opt-in launch of Nav2/SLAM Toolbox, camera and plugins; actuator arming is separate |
| `memory_service` | C++ persistent per-person, consent-gated memories |
| `gpt_bridge` | C++ local OpenAI-compatible chat bridge from `/audio/heard` to `/robot/say` |
| `student_vision` | Camera identity adapter for explicitly enrolled people only |
| `feature_plugins` | Allowlisted Python ROS feature plugins |
| `terminal_input` | Text-only development conversation input; **not** voice recognition |

I interpret “SLAMR LiDAR” as *LiDAR-based simultaneous localization and mapping* because no LiDAR make/model was specified. SLAM Toolbox consumes a driver-provided `sensor_msgs/LaserScan` on `/scan`; Nav2 supplies planning. Neither the LiDAR driver nor wheel odometry, camera driver, speech recognition, and speech synthesis can be selected correctly without your hardware choices. No motor command should ever come from LLM output.

## Install on robot

Install Ubuntu 24.04 arm64, ROS 2 Jazzy, `liblgpio-dev`, `python3-colcon-common-extensions`, `ros-jazzy-navigation2`, `ros-jazzy-nav2-bringup`, `ros-jazzy-slam-toolbox`, `libcurl4-openssl-dev`, and `nlohmann-json3-dev`. Install remaining ROS dependencies via `rosdep`. Package-specific Python dependencies are documented in their package files. Build on the Pi:

```bash
source /opt/ros/jazzy/setup.bash
cd ros2_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
ros2 launch robot_bringup robot.launch.py
```

`armed` and `navigation` default to false; even with `armed:=true`, the motor node should stay stopped without fresh scan and odometry. After integrating real drivers, odometry plus `odom->base_link` TF, LiDAR plus `base_link->laser` TF, encoder feedback, a tested physical emergency-stop and correctly wired H-bridge, inspect scans in RViz and test with wheels raised. Only then opt in:

```bash
ros2 launch robot_bringup robot.launch.py navigation:=true armed:=true
```

Motor GPIO pin defaults are 18/23 (left forward/reverse) and 24/25 (right forward/reverse). Verify pin numbering and **never attach motors directly to Pi GPIO**; use a suitable H-bridge, separate supply, common ground, fuses, and a hardwired E-stop. Override pins, wheel separation and wheel speed from ROS parameters for actual hardware. The PWM motor driver is not a closed-loop velocity controller; it cannot deliver accurate Nav2 odometry by itself. The ROS `/emergency_stop` topic is supplementary and cannot replace a hardwired safety circuit.

## Conversation and privacy

For text smoke tests run `ros2 run terminal_input terminal_input_node` in a second terminal and read `ros2 topic echo /robot/say`. To talk aloud, supply microphone STT publishing transcripts on `/audio/heard` and TTS consuming `/robot/say`; choose/measure on-device components for your latency budget. The LLM HTTP endpoint must be provided by a separately installed local inference service; **no model weights are shipped here**. Treat student identity and face embeddings as sensitive personal data: get explicit informed consent for enrollment and remembering, document retention/access/deletion, provide a no-recording mode, and obtain campus approval before deployment, especially for minors. Do not log transcripts, API keys or biometric images. Review local law and institutional policy.

`ros2_ws/README.md` contains component interfaces and additional setup details. Development on this Windows machine can run Python/static tests; ROS 2 and real GPIO hardware are absent, so a Pi build, integration test and safety validation remain required.
