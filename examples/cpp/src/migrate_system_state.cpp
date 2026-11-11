/*
 @file migrate_system_state.cpp
 @brief 系统重启示例脚本

 @description
   通过 MigrateSystemState 服务请求机器人执行系统状态迁移（如重启）。
   该接口在系统管理层实现，通过状态机机制完成安全的状态切换。

   警告：调用此接口后，系统将进入安全的重启流程，依次停止各子系统服务后执行重启。
   请确保在调用前已保存重要数据。

   ⚠ 摔机风险警告：本接口不会自动切换机器人动作状态。执行前务必先通过上层
   业务逻辑将机器人切换至趴下（Prone）或零力矩等安全姿态，否则重启过程中
   关节失去力矩支撑，机器人可能直接摔倒造成硬件损坏！

 @prerequisites
   - 系统状态管理服务必须正在运行
   - 机器人在安全状态下可执行重启
   - ⚠ 机器人已切换至趴下或零力矩等安全姿态（本接口不会自动切换动作）

 @usage
   ros2 run aimdk_examples_cpp migrate_system_state

 @example
   ros2 run aimdk_examples_cpp migrate_system_state

 @parameters
   - state: 目标系统状态（如 "Reboot"）
 */

#include "aimdk_msgs/msg/common_request.hpp"
#include "aimdk_msgs/msg/common_state.hpp"
#include "aimdk_msgs/srv/migrate_system_state.hpp"
#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <csignal>
#include <memory>
#include <string>
#include <thread>
#include <iostream>

using namespace std::chrono_literals;

namespace
{

constexpr auto kServiceWaitInterval = 2s;
constexpr auto kServiceCallTimeout  = 5s;
constexpr int kMaxRetryCount = 3;

}  // namespace

std::shared_ptr<rclcpp::Node> g_node = nullptr;

void signal_handler(int signal)
{
  if (g_node) {
    RCLCPP_INFO(g_node->get_logger(), "Received signal %d, shutting down...",
                signal);
    g_node.reset();
  }
  rclcpp::shutdown();
  exit(signal);
}

class MigrateSystemStateClient : public rclcpp::Node
{
 public:
  MigrateSystemStateClient() : Node("migrate_system_state_client")
  {
    client_ = this->create_client<aimdk_msgs::srv::MigrateSystemState>(
      "/aimdk_5Fmsgs/srv/MigrateSystemState");
    RCLCPP_INFO(this->get_logger(), "MigrateSystemState client node created.");

    while (!client_->wait_for_service(kServiceWaitInterval)) {
      if (!rclcpp::ok())
        return;
      RCLCPP_INFO(this->get_logger(), "Waiting for service...");
    }
    RCLCPP_INFO(this->get_logger(), "Service available, ready to send request.");
  }

  bool send_request(const std::string &state = "Reboot")
  {
    try {
      RCLCPP_INFO(this->get_logger(),
                  "Sending MigrateSystemState request: state=%s", state.c_str());

      for (int i = 0; i < kMaxRetryCount; ++i) {
        auto request = std::make_shared<aimdk_msgs::srv::MigrateSystemState::Request>();
        request->header.header.stamp = this->now();
        request->state = state;

        auto future = client_->async_send_request(request);
        auto retcode = rclcpp::spin_until_future_complete(
          shared_from_this(), future, kServiceCallTimeout);

        if (retcode == rclcpp::FutureReturnCode::SUCCESS) {
          auto response = future.get();
          RCLCPP_INFO(this->get_logger(), "MigrateSystemState response code: %ld",
                      response->header.header.code);
          if (response->header.header.code == 0) {
            RCLCPP_INFO(this->get_logger(),
                        "MigrateSystemState request successful. Robot is transitioning to state: %s...",
                        state.c_str());
            return true;
          }
          RCLCPP_WARN(this->get_logger(),
                      "MigrateSystemState failed with code: %ld, retrying (%d/%d)...",
                      response->header.header.code, i + 1, kMaxRetryCount);
        } else {
          RCLCPP_WARN(this->get_logger(),
                      "MigrateSystemState timed out, retrying (%d/%d)...",
                      i + 1, kMaxRetryCount);
        }
        if (i + 1 < kMaxRetryCount) {
          std::this_thread::sleep_for(1s);
        }
      }
      RCLCPP_ERROR(this->get_logger(),
                   "MigrateSystemState failed after %d retries", kMaxRetryCount);
      return false;
    } catch (const std::exception &e) {
      RCLCPP_ERROR(this->get_logger(), "MigrateSystemState exception: %s", e.what());
      return false;
    }
  }

 private:
  rclcpp::Client<aimdk_msgs::srv::MigrateSystemState>::SharedPtr client_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int choice = 0;
  std::cout << "Select system operation:" << std::endl;
  std::cout << "  1. Reboot (重启)" << std::endl;
  std::cout << "  2. Shutdown (关机)" << std::endl;
  std::cout << "Enter choice (default 1): ";
  std::cin >> choice;

  std::string state;
  if (choice == 2) {
    state = "Shutdown";
  } else {
    state = "Reboot";
  }

  auto node = std::make_shared<MigrateSystemStateClient>();
  bool success = node->send_request(state);

  rclcpp::shutdown();
  return success ? 0 : 1;
}
