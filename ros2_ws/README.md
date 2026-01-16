# ROS 2 C++ Workspace for Robot (Pi 5 + AI HAT 2)

Target:
- OS: Ubuntu 22.04 (arm64) or Raspberry Pi OS 64-bit
- ROS 2: Humble Hawksbill (LTS)
- Raspberry Pi 5 with Raspberry Pi AI HAT 2

Quick build (on Pi):
1. Install ROS 2 Humble (follow ROS 2 docs for Ubuntu 22.04 arm64).
2. Install dependencies:
   sudo apt update
   sudo apt install -y build-essential cmake git libcurl4-openssl-dev libssl-dev libasound2-dev liblgpio-dev
3. Create workspace and copy packages:
   mkdir -p ~/ros2_ws/src
   (copy packages under ros2_ws/src)
4. Build:
   cd ~/ros2_ws
   source /opt/ros/humble/setup.bash
   colcon build --symlink-install
5. Run a node:
   source install/setup.bash
   ros2 run terminal_input terminal_input_node

Notes:
- whisper.cpp: recommended for STT on-device. Build whisper.cpp separately and place its binary or library on the Pi; audio_listener package expects a CLI or library for inference.
- OpenAI: gpt_bridge uses the OpenAI HTTP API. Set OPENAI_API_KEY in the environment.
- Motor control uses lgpio (Pi 5). Ensure lgpio is installed and accessible (liblgpio-dev / lgpio-user).
- For production, run nodes under systemd and ensure audio devices and permissions are configured.