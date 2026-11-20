#include <panda_pick_demo/task_manager.h>

int main(int argc, char** argv) {
	using panda_pick_demo::TaskManager;
	using panda_pick_demo::TaskResult;

	rclcpp::init(argc, argv);
	int exit_code = 0;
	try {
		rclcpp::NodeOptions options;
		options.automatically_declare_parameters_from_overrides(true);
		auto node = rclcpp::Node::make_shared("panda_pick_node", options);
		TaskManager manager(*node, [](uint64_t, const TaskManager::Point&, const TaskManager::ReportProgress&) {
			return TaskResult{false, "backend_not_connected"};
		});
		rclcpp::spin(node);
	} catch (const std::exception& error) {
		RCLCPP_FATAL(rclcpp::get_logger("panda_pick_node"), "%s", error.what());
		exit_code = 1;
	}
	rclcpp::shutdown();
	return exit_code;
}
