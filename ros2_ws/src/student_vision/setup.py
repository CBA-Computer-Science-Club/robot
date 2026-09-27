from setuptools import setup

package_name = "student_vision"
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
    description="Opt-in local camera face-template matcher",
    license="Apache-2.0",
    entry_points={"console_scripts": [
        "student_vision_node = student_vision.node:main",
        "student_vision_manage = student_vision.cli:main",
    ]},
)
