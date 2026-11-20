from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder


def generate_launch_description():
    share = Path(get_package_share_directory("panda_pick_demo"))
    moveit_config = (
        MoveItConfigsBuilder("moveit_resources_panda")
        .robot_description(
            file_path="config/panda.urdf.xacro",
            mappings={"ros2_control_hardware_type": "mock_components"},
        )
        .planning_pipelines(pipelines=["ompl"])
        .to_moveit_configs()
    )
    return LaunchDescription([
        DeclareLaunchArgument("bringup", default_value="true", description="Start the Panda mock environment"),
        DeclareLaunchArgument("rviz", default_value="true", description="Start RViz with the mock environment"),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(str(share / "launch" / "demo.launch.py")),
            launch_arguments={"rviz": LaunchConfiguration("rviz")}.items(),
            condition=IfCondition(LaunchConfiguration("bringup")),
        ),
        Node(
            package="panda_pick_demo",
            executable="pick_place_node",
            output="screen",
            parameters=[
                moveit_config.to_dict(),
                str(share / "config" / "panda_config.yaml"),
                str(share / "config" / "task_params.yaml"),
            ],
        ),
    ])
