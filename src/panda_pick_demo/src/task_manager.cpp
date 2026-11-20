#include <panda_pick_demo/task_manager.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace panda_pick_demo {
namespace {

std::array<double, 3> readBounds(const rclcpp::Node& node, const char* name) {
	const auto values = node.get_parameter(name).as_double_array();
	if (values.size() != 3 ||
	    !std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); }))
		throw std::invalid_argument(std::string(name) + " must contain three finite coordinates");
	return {values[0], values[1], values[2]};
}

}  // namespace

TaskManager::TaskManager(rclcpp::Node& node, RunTask run_task)
  : logger_(node.get_logger())
  , run_task_(std::move(run_task))
  , min_(readBounds(node, "target_min"))
  , max_(readBounds(node, "target_max")) {
	for (std::size_t axis = 0; axis < min_.size(); ++axis) {
		if (min_[axis] > max_[axis])
			throw std::invalid_argument("target_min must be at or below target_max on each axis");
	}

	status_ = node.create_publisher<std_msgs::msg::String>("/pick_status", rclcpp::QoS(20).transient_local());
	input_ = node.create_subscription<Point>("/pick_target", rclcpp::QoS(10),
	                                        [this](const Point& target) { receiveTarget(target); });
	publishStatus(0, "IDLE", "waiting_for_target");
	worker_ = std::thread(&TaskManager::work, this);
}

TaskManager::~TaskManager() {
	{
		std::lock_guard lock(mutex_);
		stopping_ = true;
	}
	work_ready_.notify_one();
	worker_.join();
}

void TaskManager::receiveTarget(const Point& target) {
	std::lock_guard lock(mutex_);
	const auto id = next_id_++;
	const std::array coordinates{target.x, target.y, target.z};
	if (!std::all_of(coordinates.begin(), coordinates.end(), [](double value) { return std::isfinite(value); })) {
		publishStatus(id, "REJECTED", "non_finite_target");
		return;
	}
	for (std::size_t axis = 0; axis < coordinates.size(); ++axis) {
		if (coordinates[axis] < min_[axis] || coordinates[axis] > max_[axis]) {
			publishStatus(id, "REJECTED", "target_out_of_bounds");
			return;
		}
	}
	if (busy_) {
		publishStatus(id, "REJECTED", "robot_busy");
		return;
	}

	busy_ = true;
	pending_ = Request{id, target};
	publishStatus(id, "ACCEPTED", "target_received");
	RCLCPP_INFO(logger_, "Target %llu in panda_link0: x=%.6f y=%.6f z=%.6f",
	            static_cast<unsigned long long>(id), target.x, target.y, target.z);
	work_ready_.notify_one();
}

void TaskManager::work() {
	while (true) {
		Request request;
		{
			std::unique_lock lock(mutex_);
			work_ready_.wait(lock, [this] { return stopping_ || pending_.has_value(); });
			if (stopping_)
				return;
			request = *pending_;
			pending_.reset();
		}

		TaskResult result{false, "internal_error"};
		try {
			result = run_task_(request.id, request.target, [this, id = request.id](const char* state, const char* detail) {
				publishStatus(id, state, detail);
			});
		} catch (const std::exception& error) {
			RCLCPP_ERROR(logger_, "Task %llu: %s", static_cast<unsigned long long>(request.id), error.what());
		}

		std::lock_guard lock(mutex_);
		if (stopping_ || !rclcpp::ok())
			return;
		publishStatus(request.id, result.succeeded ? "SUCCEEDED" : "FAILED", result.detail, result.moveit_code);
		busy_ = false;
	}
}

void TaskManager::publishStatus(uint64_t id, const char* state, const std::string& detail,
                                std::optional<int32_t> moveit_code) {
	if (!rclcpp::ok())
		return;
	std_msgs::msg::String message;
	message.data = "id=" + std::to_string(id) + " state=" + state + " detail=" + detail;
	if (moveit_code)
		message.data += " moveit_code=" + std::to_string(*moveit_code);
	status_->publish(message);
	RCLCPP_INFO(logger_, "%s", message.data.c_str());
}

}  // namespace panda_pick_demo
