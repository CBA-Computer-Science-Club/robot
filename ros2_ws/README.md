# ROS 2 workspace (Ubuntu 24.04 arm64 / ROS 2 Jazzy)

See [repository README](../README.md) for installation and hardware safety. The original `Humble + AI HAT 2` note was a placeholder; no HAT integration or speech drivers are supplied. Build from this directory after sourcing `/opt/ros/jazzy/setup.bash` and installing package dependencies. This workspace is not intended to build under Windows without ROS 2.

## Nodes and data flow

```text
LiDAR driver -> /scan ---------+-> SLAM Toolbox -> /map + map->odom
encoder/IMU driver -> /odom ---+-> Nav2 -> /cmd_vel -> motor_controller -> H-bridge
camera driver -> /camera/image_raw -> student_vision -> /people/identity (opt-in)
microphone/STT -> /audio/heard -> gpt_bridge <-> local LLM HTTP endpoint
                                        |       <-> memory_service (explicit opt-in)
                                        +-> /robot/say -> external TTS/speaker
feature_plugins -> allowlisted Python feature nodes (disabled by default)
```

`/scan` and `/odom` must be real timely data. TF `odom->base_link` and `base_link->laser` must be derived from a calibrated physical robot; this repo intentionally does not publish fictitious transforms, odometry or camera input. Nav2 and SLAM Toolbox are integration targets rather than a complete navigation configuration. The robot should not be armed or driven around people until collision, acceleration, motor-direction, odometry, timing and E-stop behavior are verified on its exact hardware. A ROS software watchdog is not a substitute for a physical E-stop.

## Start safely

```bash
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
mkdir -p "$HOME/.local/share/robot" && chmod 700 "$HOME/.local/share/robot"
ros2 launch robot_bringup robot.launch.py memory_path:="$HOME/.local/share/robot/memories.json"
```

Launch defaults: `armed:=False`, `navigation:=False`, `camera:=False`, `plugins:=False`. `memory_path` chooses persistent local JSON, `model` chooses a model **already installed** in the loopback inference server. Launching `camera:=true` starts the vision node with `enabled:=true` but requires camera/OpenCV configuration and consented enrollments. Launching `plugins:=true` starts the host with an empty allowlist; install/review/approve plugins before expecting output. `gpt_bridge` defaults to a localhost OpenAI-compatible endpoint but does not launch or install a server or weights. For production configure memory in a private persistent directory with file permissions and isolate ROS graph access; see each package's README.

### Explicit person memory

`gpt_bridge` never remembers automatically. Only `Remember: <fact>` may write. Its defaults **disable memory**; for a verified single-person session with explicit consent, launch with `memory_consent:=true consented_person_id:=person_123 identity_mode:=operator_bound` and bind the microphone to that consenting person. Use `identity_mode:=trusted_topic` only behind an authenticated identity provider (the camera alone is not one). See `gpt_bridge/README.md` and `memory_service/README.md` for per-person retrieval and `memory/forget` deletion. Recognition is not authentication.

### Tests runnable without ROS 2

```bash
python -m unittest discover -s src/motor_controller/tests -p 'test_*.py'
python -m unittest discover -s src/robot_bringup/tests -p 'test_*.py'
python -m unittest discover -s src/memory_service/tests -p 'test_interfaces.py'
PYTHONPATH=src/student_vision:src/feature_plugins python -m unittest discover -s src/student_vision/test -p 'test_*.py'
PYTHONPATH=src/student_vision:src/feature_plugins python -m unittest discover -s src/feature_plugins/test -p 'test_*.py'
cmake -S src/motor_controller/tests -B /tmp/motor-tests && cmake --build /tmp/motor-tests && ctest --test-dir /tmp/motor-tests --output-on-failure
cmake -S src/gpt_bridge/tests -B /tmp/chat-tests && cmake --build /tmp/chat-tests && ctest --test-dir /tmp/chat-tests --output-on-failure
cmake -S src/memory_service/tests -B /tmp/memory-tests && cmake --build /tmp/memory-tests && ctest --test-dir /tmp/memory-tests --output-on-failure
```

The memory standalone build requires nlohmann JSON C++ headers. On Windows with a multi-config CMake generator pass `-C Debug` to ctest. ROS integration must be tested on Ubuntu with the exact Pi hardware, physical stop, camera, encoders and LiDAR connected.
