# feature_plugins (ROS 2 Jazzy)

Python entry points are **disabled by default**. Only an *installed, trusted* Python distribution whose group, distribution name, entry-point name and target exactly match `approved_plugins` can be imported and started. A missing/duplicate approval fails startup before loading any plugin; random ROS topics or messages cannot authorize code. Example (built-in example installed by this package):

```bash
colcon build --packages-select feature_plugins
source install/setup.bash
ros2 run feature_plugins feature_plugins_node --ros-args -p "approved_plugins:=['feature_plugins:pulse=feature_plugins.example:HeartbeatPlugin']"
ros2 topic echo /feature_plugins/pulse
```

Approval syntax: `distribution:entry_name=module:Class`. For your own trusted Python ament_python distribution, add to its `setup.py`:

```python
entry_points={"robot.feature_plugins": ["my_feature = my_package.extension:MyFeature"]}
```

Implement `MyFeature.start(api)` (optional `stop()` on shutdown). The facade only offers `api.publish_text(str)` to `feature_plugins/<entry_name>` (`std_msgs/String`, max 4096 chars) and `api.every(seconds, callback)` (0.1–3600 seconds). Plugin names must be safe ROS names; arbitrary topic creation is not provided by the facade. Add the exact distribution/name/target approval above after code review and install from a trusted source. Approval requires a restart. Never approve user-supplied package names or change the allowlist from untrusted messages.

**Not a security sandbox:** Python code can use imports, inspect objects, open files or access ROS directly. The facade scopes the *supported extension interface*, not process privileges. Run only reviewed/trusted packages; isolate them at OS/process level if hostile code is possible. Plugin callbacks share the node executor; long-running callbacks block others.

Windows host unit tests (no ROS required): `PYTHONPATH='ros2_ws/src/student_vision;ros2_ws/src/feature_plugins' python -m unittest discover -s ros2_ws/src/feature_plugins/test -p 'test_*.py'`. Use colon instead of semicolon in Linux `PYTHONPATH`.
