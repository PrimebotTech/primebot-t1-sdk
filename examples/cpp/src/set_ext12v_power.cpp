/*
 @file set_ext12v_power.cpp
 @brief 外挂算力板供电控制示例脚本

 @description
   通过 SetExt12vPower 服务控制机器人 XT30 接口的外挂算力板供电状态。
   支持开启（enable: true）或关闭（enable: false）外部计算板的电源供应。

   注意：关闭供电前，请确保外挂算力板已正常关机或进入安全状态。

 @prerequisites
   - 外挂算力板供电服务必须正在运行
   - XT30 接口已正确连接外部计算板

 @usage
   ros2 run aimdk_examples_cpp set_ext12v_power [on|off]

 @example
   ros2 run aimdk_examples_cpp set_ext12v_power on    # 开启供电
   ros2 run aimdk_examples_cpp set_ext12v_power off   # 关闭供电

 @parameters
   - enable: 供电使能（true=开启，false=关闭）
 */

#include "aimdk_msgs/msg/common_request.hpp"
#include "aimdk_msgs/msg/common_state.hpp"
#include "aimdk_msgs/srv/set_ext12v_power.hpp"
#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

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

class SetExt12vPowerClient : public rclcpp::Node
{
 public:
  SetExt12vPowerClient() : Node("set_ext12v_power_client")
  {
    client_ = this->create_client<aimdk_msgs::srv::SetExt12vPower>(
      "/aimdk_5Fmsgs/srv/SetExt12vPower");
    RCLCPP_INFO(this->get_logger(), "SetExt12vPower client node created.");

    while (!client_->wait_for_service(kServiceWaitInterval)) {
      if (!rclcpp::ok())
        return;
      RCLCPP_INFO(this->get_logger(), "Waiting for service...");
    }
    RCLCPP_INFO(this->get_logger(), "Service available, ready to send request.");
  }

  bool send_request(bool enable)
  {
    try {
      RCLCPP_INFO(this->get_logger(),
                  "Sending SetExt12vPower request: enable=%s",
                  enable ? "true" : "false");

      for (int i = 0; i < kMaxRetryCount; ++i) {
        auto request = std::make_shared<aimdk_msgs::srv::SetExt12vPower::Request>();
        request->request.header.stamp = this->now();
        request->enable = enable;

        auto future = client_->async_send_request(request);
        auto status = rclcpp::spin_until_future_complete(
            shared_from_this(), future, kServiceCallTimeout);
        if (status == rclcpp::FutureReturnCode::SUCCESS) {
          auto response = future.get();
          RCLCPP_INFO(this->get_logger(), "SetExt12vPower response code: %ld",
                      response->header.header.code);
          if (response->header.header.code == 0) {
            RCLCPP_INFO(this->get_logger(),
                        "SetExt12vPower request successful. External compute board power: %s",
                        enable ? "ON" : "OFF");
            return true;
          }
          RCLCPP_WARN(this->get_logger(),
                      "SetExt12vPower failed with code: %ld, retrying (%d/%d)...",
                      response->header.header.code, i + 1, kMaxRetryCount);
        } else {
          RCLCPP_WARN(this->get_logger(),
                      "SetExt12vPower timed out, retrying (%d/%d)...",
                      i + 1, kMaxRetryCount);
        }
        if (i + 1 < kMaxRetryCount) {
          std::this_thread::sleep_for(1s);
        }
      }
      RCLCPP_ERROR(this->get_logger(),
                   "SetExt12vPower failed after %d retries", kMaxRetryCount);
      return false;
    } catch (const std::exception &e) {
      RCLCPP_ERROR(this->get_logger(), "SetExt12vPower exception: %s", e.what());
      return false;
    }
  }

 private:
  rclcpp::Client<aimdk_msgs::srv::SetExt12vPower>::SharedPtr client_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // 解析命令行参数
  bool enable = true;  // 默认开启
  if (argc > 1) {
    std::string arg = argv[1];
    if (arg == "on" || arg == "true" || arg == "1") {
      enable = true;
    } else if (arg == "off" || arg == "false" || arg == "0") {
      enable = false;
    } else {
      std::cout << "Usage: " << argv[0] << " [on|off]" << std::endl;
      std::cout << "  on   : 开启外挂算力板供电 (default)" << std::endl;
      std::cout << "  off  : 关闭外挂算力板供电" << std::endl;
      rclcpp::shutdown();
      return 1;
    }
  }

  auto node = std::make_shared<SetExt12vPowerClient>();
  bool success = node->send_request(enable);

  rclcpp::shutdown();
  return success ? 0 : 1;
}
