/*
 @file stop_audio_play.cpp
 @brief 停止音频播放示例脚本

 @description
   通过 StopAudioPlay 服务停止机器人当前正在播放的音频。
   该接口会停止所有活跃的音频播放会话。

 @prerequisites
   - 音频管理服务必须正在运行

 @usage
   ros2 run aimdk_examples_cpp stop_audio_play

 @example
   ros2 run aimdk_examples_cpp stop_audio_play
 */

#include "aimdk_msgs/msg/common_request.hpp"
#include "aimdk_msgs/msg/common_state.hpp"
#include "aimdk_msgs/srv/stop_audio_play.hpp"
#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <csignal>
#include <memory>
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

class StopAudioPlayClient : public rclcpp::Node
{
 public:
  StopAudioPlayClient() : Node("stop_audio_play_client")
  {
    client_ = this->create_client<aimdk_msgs::srv::StopAudioPlay>(
      "/aimdk_5Fmsgs/srv/StopAudioPlay");
    RCLCPP_INFO(this->get_logger(), "StopAudioPlay client node created.");

    while (!client_->wait_for_service(kServiceWaitInterval)) {
      if (!rclcpp::ok())
        return;
      RCLCPP_INFO(this->get_logger(), "Waiting for service...");
    }
    RCLCPP_INFO(this->get_logger(), "Service available, ready to send request.");
  }

  bool send_request()
  {
    try {
      for (int i = 0; i < kMaxRetryCount; ++i) {
        auto request = std::make_shared<aimdk_msgs::srv::StopAudioPlay::Request>();
        request->request.header.stamp = this->now();

        RCLCPP_INFO(this->get_logger(),
                    "Sending StopAudioPlay request (%d/%d)...", i + 1,
                    kMaxRetryCount);

        auto future = client_->async_send_request(request);

        if (rclcpp::spin_until_future_complete(this->get_node_base_interface(),
                                               future, kServiceCallTimeout)
            == rclcpp::FutureReturnCode::SUCCESS)
        {
          auto response = future.get();
          RCLCPP_INFO(this->get_logger(), "StopAudioPlay response code: %ld",
                      response->response.header.code);
          if (response->response.header.code == 0) {
            RCLCPP_INFO(this->get_logger(),
                        "StopAudioPlay request successful. Audio playback stopped.");
            return true;
          }
          RCLCPP_WARN(this->get_logger(),
                      "StopAudioPlay failed with code: %ld, retrying...",
                      response->response.header.code);
        } else {
          RCLCPP_WARN(this->get_logger(),
                      "StopAudioPlay timed out, retrying (%d/%d)...",
                      i + 1, kMaxRetryCount);
        }
        std::this_thread::sleep_for(1s);
      }
      RCLCPP_ERROR(this->get_logger(),
                   "StopAudioPlay failed after %d retries", kMaxRetryCount);
      return false;
    } catch (const std::exception &e) {
      RCLCPP_ERROR(this->get_logger(), "StopAudioPlay exception: %s", e.what());
      return false;
    }
  }

 private:
  rclcpp::Client<aimdk_msgs::srv::StopAudioPlay>::SharedPtr client_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  auto node = std::make_shared<StopAudioPlayClient>();
  bool success = node->send_request();

  rclcpp::shutdown();
  return success ? 0 : 1;
}
