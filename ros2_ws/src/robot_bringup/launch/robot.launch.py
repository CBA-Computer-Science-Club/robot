from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    armed = LaunchConfiguration('armed')
    navigation = LaunchConfiguration('navigation')
    camera = LaunchConfiguration('camera')
    plugins = LaunchConfiguration('plugins')
    memory_path = LaunchConfiguration('memory_path')
    model = LaunchConfiguration('model')
    endpoint = LaunchConfiguration('endpoint')
    memory_consent = LaunchConfiguration('memory_consent')
    consented_person_id = LaunchConfiguration('consented_person_id')
    identity_mode = LaunchConfiguration('identity_mode')
    # Do not start actuators by default. /scan, /odom, odom->base_link TF,
    # base_link->laser TF and camera/image_raw must come from actual drivers.
    return LaunchDescription([
        DeclareLaunchArgument('armed', default_value='False', description='Explicitly arm H-bridge after safety checks'),
        DeclareLaunchArgument('navigation', default_value='False', description='Start SLAM Toolbox + Nav2'),
        DeclareLaunchArgument('camera', default_value='False', description='Start opt-in face recognition'),
        DeclareLaunchArgument('plugins', default_value='False', description='Start allowlisted feature plugins'),
        DeclareLaunchArgument('memory_path', default_value='memory_service.json', description='Private persistent memory file path'),
        DeclareLaunchArgument('model', default_value='llama3.2', description='Model already installed in local inference runtime'),
        DeclareLaunchArgument('endpoint', default_value='http://127.0.0.1:11434/v1/chat/completions'),
        DeclareLaunchArgument('memory_consent', default_value='False', description='Operator-attested consent for this session'),
        DeclareLaunchArgument('consented_person_id', default_value='', description='Operator-verified consenting person ID'),
        DeclareLaunchArgument('identity_mode', default_value='disabled', description='disabled, operator_bound, or authenticated trusted_topic'),
        Node(package='memory_service', executable='memory_service_node', output='screen',
             parameters=[{'storage_path': memory_path}]),
        Node(package='gpt_bridge', executable='gpt_bridge_node', output='screen',
             parameters=[{'model': model, 'endpoint': endpoint,
                          'memory_consent': ParameterValue(memory_consent, value_type=bool),
                          'consented_person_id': consented_person_id, 'identity_mode': identity_mode}]),
        Node(package='motor_controller', executable='motor_controller_node', output='screen',
             parameters=[{'armed': ParameterValue(armed, value_type=bool)}]),
        Node(package='student_vision', executable='student_vision_node', output='screen',
             parameters=[{'enabled': True}], condition=IfCondition(camera)),
        Node(package='feature_plugins', executable='feature_plugins_node', output='screen',
             condition=IfCondition(plugins)),
        IncludeLaunchDescription(PythonLaunchDescriptionSource(PathJoinSubstitution([
            FindPackageShare('slam_toolbox'), 'launch', 'online_async_launch.py'])),
            condition=IfCondition(navigation)),
        IncludeLaunchDescription(PythonLaunchDescriptionSource(PathJoinSubstitution([
            FindPackageShare('nav2_bringup'), 'launch', 'navigation_launch.py'])),
            condition=IfCondition(navigation)),
    ])
