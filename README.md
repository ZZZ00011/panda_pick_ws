# Panda Pick Demo

适用于 Ubuntu 22.04 和 ROS 2 Humble。包名为 `panda_pick_demo`，工作空间为 `~/panda_pick_ws`。

## 官方示例与本包

开始时工作空间的 `src/` 为空，ROS 从 `/opt/ros/humble` 加载 MTC 0.1.3。安装包中的两份 Launch、Panda YAML 和 `package.xml` 与 0.1.3 发布源码一致。对应的 C++ 和 `CMakeLists.txt` 也已对照 Humble 分支检查。 [MTC 0.1.3 发布源码](https://github.com/ros2-gbp/moveit_task_constructor-release/tree/984cb43a0c07cbb7fbe2e02dd8e38e8d50f4a72b/demo)

| 项目 | 官方默认 | 本包默认 |
| --- | --- | --- |
| 抓取对象 | `object`，竖直圆柱，高 0.25 m、半径 0.02 m | 同左 |
| 抓取点 | 圆柱底面中心 `(0.50, -0.25, 0.00)`，几何中心 z 为 0.125 m | 同左 |
| 放置点 | 圆柱底面目标 `(0.60, -0.15, 0.00)` | `(0.55, -0.10, 0.00)` |
| 桌面 | `table`，长宽高 `[0.4, 0.5, 0.1]` m，桌面中心 `(0.50, -0.25, 0.00)` | 同左，可在 Launch 关闭 |
| 执行 | 生成规划后保留 MTC 解供 RViz 查看。启用 ROS 参数 `execute` 后提交第一个完整解 | 默认只规划，`execute:=true` 将第一个完整解提交给本地模拟控制器 |

上述位置使用 `world`，由启动文件发布与 `panda_link0` 对齐的静态变换。官方代码从底面 z 加半高得到圆柱中心，放置时再加 `place_surface_offset=0.0001` m。桌面以顶面为参考，几何中心位于 z = −0.05 m。

任务仍用 `panda_arm`、`hand` 与 `panda_link8`。它从当前场景出发，用 OMPL 做段间连接，每 15° 采样一个圆柱抓取方向，并结合夹爪抓取偏移计算 IK。接近、抬升、下降和撤离使用笛卡尔路径。夹爪关闭后附着物体，放置时打开夹爪并分离物体，最后返回 `ready`。完整解搜索默认最多收集 10 个解，`planning_timeout` 默认 20 秒。MTC 在阶段计算之间检查该时限，没有完整解时会报告 MoveIt 结果。

## 构建

```bash
cd ~/panda_pick_ws
source /opt/ros/humble/setup.bash
rosdep check --from-paths src --ignore-src --rosdistro humble
CMAKE_BUILD_PARALLEL_LEVEL=2 colcon build --packages-select panda_pick_demo \
  --event-handlers console_direct+ --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
```

如果 `rosdep check` 报缺少依赖，可在已初始化 rosdep 的环境中用 `rosdep install --from-paths src --ignore-src --rosdistro humble -y` 安装包清单中的依赖。

## 启动

两个终端都加载同一套工作空间环境，用独立 ROS 域隔离本次演示。

```bash
cd ~/panda_pick_ws
source /opt/ros/humble/setup.bash
source install/setup.bash
export ROS_LOCALHOST_ONLY=1 ROS_DOMAIN_ID=74
```

终端 1 启动 MoveGroup、Panda 模拟控制器和 RViz。无图形会话时加 `rviz:=false`。

```bash
ros2 launch panda_pick_demo demo.launch.py
```

等终端 1 打印 `You can start planning now!`，并确认 `joint_state_broadcaster`、`panda_arm_controller` 和 `panda_hand_controller` 均已激活，再在终端 2 启动任务。

```bash
ros2 launch panda_pick_demo run.launch.py
```

出现 `Planning succeeded with ... complete solution(s)` 和 `Execution disabled` 后，在 RViz 的 **Motion Planning Tasks** 中选中 `pick_place_task` 及一个完整解，查看抓放和附着动画。每个测试结束后先在终端 2 按 Ctrl+C，再关闭终端 1。改变桌面开关或需要复位关节和场景时，重新启动终端 1。若同一场景此前已添加桌面，关闭开关后需重启环境来清除该桌面。

用同一套命令只修改放置点。

```bash
ros2 launch panda_pick_demo run.launch.py 'place_pose:=[0.58,-0.12,0,0,0,0]'
```

以下组合修改抓取位置、尺寸和放置位置，并执行完整解。

```bash
ros2 launch panda_pick_demo run.launch.py \
  'object_pose:=[0.47,-0.22,0,0,0,0]' \
  'object_dimensions:=[0.20,0.018]' \
  'place_pose:=[0.58,-0.12,0,0,0,0]' \
  execute:=true
```

| 启动参数 | 含义 |
| --- | --- |
| `object_pose` | 抓取时圆柱底面位姿，`[x,y,z,rx,ry,rz]`，单位 m 和 rad |
| `object_dimensions` | 圆柱的 `[height,radius]`，单位 m，两项均为正数 |
| `place_pose` | 放置时圆柱底面目标。与 `object_pose` 使用同一格式 |
| `spawn_table` | 是否添加演示桌面，默认 `true` |
| `execute` | 是否将第一个完整解交给 MoveIt，默认 `false` |

竖直圆柱的可复现用例中，`rx` 和 `ry` 均为零。沿用官方代码，几何高度偏移沿参考系的 Z 轴计算。更改默认值、桌面尺寸或参考帧可编辑 [panda_config.yaml](src/panda_pick_demo/config/panda_config.yaml)，重新运行 colcon 并启动节点。显式 Launch 参数覆盖对应默认值，任务在启动时读取一次。

## 测试清单

每项在新启动的假硬件环境中运行。运行参数、状态码和实际场景可用下面的命令检查。查询中的 `20` 同时请求世界物体几何和附着物体。在任务启动或执行结束后，用同一 ROS 域中的第三个终端查询。

```bash
ros2 launch panda_pick_demo run.launch.py --show-args
ros2 action list
ros2 service call /get_planning_scene moveit_msgs/srv/GetPlanningScene \
  '{components: {components: 20}}'
```

| 测试 | 预期与本机结果 |
| --- | --- |
| 默认只规划 | 找到完整解，`Execution disabled`，场景中的圆柱仍在抓取点。 |
| 只改抓取点 `object_pose:=[0.47,-0.22,0,0,0,0]` | 规划成功，场景中的圆柱中心为 `(0.47,-0.22,0.125)`。 |
| 只改尺寸 `object_dimensions:=[0.20,0.018]` | 规划成功，场景尺寸为 `[0.20,0.018]`，圆柱中心 z 为 0.10 m。 |
| 只改放置点 `place_pose:=[0.58,-0.12,0,0,0,0]` | 规划成功。从 MTC 的完整解中读取到附着、分离和最终圆柱中心 `(0.58,-0.12,0.1251)`。 |
| 关闭桌面 `spawn_table:=false` | 在新场景中规划成功，场景只含圆柱。 |
| 默认和组合参数下的 `execute:=true` | MoveIt 返回 `1`，两次执行分别抵达约 `(0.55,-0.10,0.1251)` 和 `(0.58,-0.12,0.1001)`。场景中圆柱已分离。 |
| 非法尺寸 `object_dimensions:=[-0.20,0.018]` | Launch 在启动任务前报错并退出。 |
| 无 MoveGroup | 单独运行 `run.launch.py`。等待场景服务最多约 5 秒，任务进程报错并以状态码 1 退出。 |
| 超出工作空间 `place_pose:=[3,0,0,0,0,0] execute:=true` | 任务报告 `TIMED_OUT (-6)` 并以状态码 1 退出，未发送执行请求。 |
| 模拟控制器拒绝执行 | 用下方命令停用假机械臂控制器，再启动任务并传入 `execute:=true`。任务进程报告执行失败并以状态码 1 退出。 |
| RViz 图形回放 | 待交互验证。在 **Motion Planning Tasks** 选中完整解，检查圆柱在抬升时跟随夹爪，在分离后留在目标位置。 |

如需用命令行核对规划解的放置点，运行 `ros2 topic echo --once --qos-durability transient_local /statistics`，读取 `task_id` 和根阶段 `solved` 中的第一个 ID。调用 `ros2 service call /get_solution_<task_id> moveit_task_constructor_msgs/srv/GetSolution '{solution_id: <id>}'`，查看返回的各段 `scene_diff`。最终放置坐标位于分离阶段重新加入世界的圆柱对象中。

仅在独立的假硬件 ROS 域进行控制器拒绝测试。执行测试后重启终端 1 即可恢复。

```bash
ros2 service call /controller_manager/switch_controller \
  controller_manager_msgs/srv/SwitchController \
  '{deactivate_controllers: [panda_arm_controller], strictness: 2, timeout: {sec: 5}}'
```

本机原始构建日志在 `log/latest_build/panda_pick_demo/`，运行日志在 `log/verify/`。场景服务的延迟响应测试记录在 `07-delayed-scene/`，第二次请求收到响应并继续初始化任务。测试中，系统安装的 MoveIt 2.5.10 `move_group` 在退出时出现过段错误，堆栈指向轨迹执行管理器析构。规划结果和执行后的场景均在退出前核对。示例场景使用参数指定的几何体，启动日志还会提示未配置三维传感器。

## RViz 逻辑抓取与物理抓取

- **逻辑抓取**。MTC 在规划场景中将圆柱附着到 `panda_link8`，据此计算带物体的碰撞和运动，分离后将圆柱放回世界场景。RViz 可以回放这些场景变化。开启执行时，`mock_components/GenericSystem` 更新模拟关节状态，MTC 执行能力把相应场景变化应用到 MoveGroup。
- **物理抓取**。物体靠真实夹爪的接触力和摩擦搬运，或在物理仿真中由接触模型、质量、重力和控制器共同决定运动。需要用传感反馈或物理仿真状态确认夹持、滑移和放置。此工作空间已验证无界面规划和假硬件执行，RViz 图形交互及物理抓取待验证。

## 版本记录

根目录的本地 Git 仓库按变更记录构建与验证节点。用 `git log --oneline --reverse` 查看历史，用 `git show <提交号>` 检查具体变更。
