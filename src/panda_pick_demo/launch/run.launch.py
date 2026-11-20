import math
from pathlib import Path

import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder


TASK_ARGUMENTS = {
    "object_pose": (6, "Pick cylinder base [x,y,z,rx,ry,rz] in world (m, rad)"),
    "object_dimensions": (2, "Cylinder [height,radius] (m), both positive"),
}


def parse_array(name, argument, size):
    try:
        values = yaml.safe_load(argument)
    except yaml.YAMLError as error:
        raise ValueError(f"{name} must be a YAML array of {size} finite numbers") from error
    if (
        not isinstance(values, list)
        or len(values) != size
        or any(type(value) not in (int, float) or not math.isfinite(value) for value in values)
    ):
        raise ValueError(f"{name} must be a YAML array of {size} finite numbers")
    return [float(value) for value in values]


def launch_task(context):
    moveit_config = (
        MoveItConfigsBuilder("moveit_resources_panda")
        .robot_description(file_path="config/panda.urdf.xacro")
        .planning_pipelines(pipelines=["ompl"])
        .to_moveit_configs()
    )
    config = Path(get_package_share_directory("panda_pick_demo")) / "config" / "panda_config.yaml"
    overrides = {}
    for name, (size, _) in TASK_ARGUMENTS.items():
        argument = LaunchConfiguration(name).perform(context)
        if argument:
            overrides[name] = parse_array(name, argument, size)
    if "object_dimensions" in overrides and min(overrides["object_dimensions"]) <= 0:
        raise ValueError("object_dimensions height and radius must be positive")

    return [
        Node(
            package="panda_pick_demo",
            executable="pick_place_demo",
            output="screen",
            parameters=[
                moveit_config.robot_description,
                moveit_config.robot_description_semantic,
                moveit_config.robot_description_kinematics,
                moveit_config.joint_limits,
                moveit_config.planning_pipelines,
                str(config),
                overrides,
            ],
        )
    ]


def generate_launch_description():
    return LaunchDescription([
        *(
            DeclareLaunchArgument(name, default_value="", description=f"{description}. Empty uses panda_config.yaml.")
            for name, (_, description) in TASK_ARGUMENTS.items()
        ),
        OpaqueFunction(function=launch_task),
    ])
