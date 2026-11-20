#include <panda_pick_demo/pick_place_task.h>
#include <panda_pick_demo/task_manager.h>

namespace {
using panda_pick_demo::TaskManager;
using panda_pick_demo::TaskResult;

TaskResult pick(const rclcpp::Node::SharedPtr& node, pick_place_task_demo::Params params, uint64_t id,
                const TaskManager::Point& target, const TaskManager::ReportProgress& report) {
	params.object_pose[0] = target.x;
	params.object_pose[1] = target.y;
	params.object_pose[2] = target.z;
	try {
		if (!panda_pick_demo::setupTargetScene(node, params))
			return {false, "object_still_attached"};
	} catch (const std::exception& error) {
		RCLCPP_ERROR(node->get_logger(), "Scene update failed for task %llu: %s",
		             static_cast<unsigned long long>(id), error.what());
		return {false, "scene_update_failed"};
	}

	report("PLANNING", "started");
	panda_pick_demo::PickPlaceTask task("pick_place_task_" + std::to_string(id));
	if (!task.init(node, params))
		return {false, "task_initialization_failed"};
	const auto plan = task.plan(params.max_solutions);
	if (!plan)
		return {false, "planning_failed", plan.val};
	if (!rclcpp::ok())
		return {false, "interrupted"};

	report("EXECUTING", "started");
	const auto execution = task.execute();
	if (!execution)
		return {false, "execution_failed", execution.val};
	try {
		if (!panda_pick_demo::objectDetachedInWorld(node, params.object_name))
			return {false, "object_release_unconfirmed", execution.val};
	} catch (const std::exception& error) {
		RCLCPP_ERROR(node->get_logger(), "Release verification failed for task %llu: %s",
		             static_cast<unsigned long long>(id), error.what());
		return {false, "scene_verification_failed", execution.val};
	}
	return {true, "task_completed", execution.val};
}
}  // namespace

int main(int argc, char** argv) {
	rclcpp::init(argc, argv);
	int exit_code = 0;
	try {
		rclcpp::NodeOptions options;
		options.automatically_declare_parameters_from_overrides(true);
		auto node = rclcpp::Node::make_shared("panda_pick_node", options);
		const pick_place_task_demo::ParamListener listener(node);
		const auto params = listener.get_params();
		TaskManager manager(*node, [node, params](uint64_t id, const auto& target, const auto& report) {
			return pick(node, params, id, target, report);
		});
		rclcpp::spin(node);
	} catch (const std::exception& error) {
		RCLCPP_FATAL(rclcpp::get_logger("panda_pick_node"), "%s", error.what());
		exit_code = 1;
	}
	rclcpp::shutdown();
	return exit_code;
}
