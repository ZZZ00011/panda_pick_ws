#pragma once

#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <geometry_msgs/msg/point.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

namespace panda_pick_demo {

struct TaskResult {
	bool succeeded;
	std::string detail;
	std::optional<int32_t> moveit_code = std::nullopt;
};

class TaskManager {
public:
	using Point = geometry_msgs::msg::Point;
	using ReportProgress = std::function<void(const char* state, const char* detail)>;
	using RunTask = std::function<TaskResult(uint64_t id, const Point&, const ReportProgress&)>;

	TaskManager(rclcpp::Node& node, RunTask run_task);
	~TaskManager();

private:
	struct Request {
		uint64_t id;
		Point target;
	};

	void receiveTarget(const Point& target);
	void work();
	void publishStatus(uint64_t id, const char* state, const std::string& detail,
	                   std::optional<int32_t> moveit_code = std::nullopt);

	rclcpp::Logger logger_;
	RunTask run_task_;
	std::array<double, 3> min_;
	std::array<double, 3> max_;
	rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_;
	rclcpp::Subscription<Point>::SharedPtr input_;
	std::mutex mutex_;
	std::condition_variable work_ready_;
	std::optional<Request> pending_;
	uint64_t next_id_ = 1;
	bool busy_ = false;
	bool stopping_ = false;
	std::thread worker_;
};

}  // namespace panda_pick_demo
