from setuptools import setup

package_name = "feature_plugins"
setup(
    name=package_name,
    version="0.1.0",
    packages=[package_name],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="Robot maintainers",
    maintainer_email="maintainers@example.invalid",
    description="Explicit allowlist for trusted Python ROS feature plugins",
    license="Apache-2.0",
    entry_points={
        "console_scripts": ["feature_plugins_node = feature_plugins.node:main"],
        "robot.feature_plugins": ["pulse = feature_plugins.example:HeartbeatPlugin"],
    },
)
