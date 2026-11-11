#!/usr/bin/env python3

"""
停止音频播放示例脚本

Description:
  通过 StopAudioPlay 服务停止机器人当前正在播放的音频。
  该接口会停止所有活跃的音频播放会话。

Prerequisites:
  - 音频管理服务必须正在运行

Usage:
  python3 stop_audio_play.py

Example:
  python3 stop_audio_play.py
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_services_default

from aimdk_msgs.srv import StopAudioPlay


class StopAudioPlayClient(Node):
    def __init__(self):
        super().__init__("stop_audio_play_client")

        self.client = self.create_client(
            StopAudioPlay,
            "/aimdk_5Fmsgs/srv/StopAudioPlay",
            qos_profile=qos_profile_services_default,
        )

        self.get_logger().info("StopAudioPlay client node created.")

        while not self.client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info("Waiting for service...")

        self.get_logger().info("Service available, ready to send request.")

    def send_request(self) -> bool:
        max_retry_count = 3
        for i in range(max_retry_count):
            request = StopAudioPlay.Request()
            request.request.header.stamp = self.get_clock().now().to_msg()

            self.get_logger().info(
                f"Sending StopAudioPlay request ({i + 1}/{max_retry_count})..."
            )

            future = self.client.call_async(request)
            rclpy.spin_until_future_complete(self, future, timeout_sec=5.0)

            if future.done():
                response = future.result()
                if response is not None:
                    self.get_logger().info(
                        f"StopAudioPlay response code: {response.response.header.code}"
                    )
                    if response.response.header.code == 0:
                        self.get_logger().info(
                            "StopAudioPlay request successful. Audio playback stopped."
                        )
                        return True
                    else:
                        self.get_logger().warn(
                            f"StopAudioPlay failed with code: {response.response.header.code}, "
                            f"retrying ({i + 1}/{max_retry_count})..."
                        )
                else:
                    self.get_logger().warn(
                        f"StopAudioPlay returned None, retrying ({i + 1}/{max_retry_count})..."
                    )
            else:
                self.get_logger().warn(
                    f"StopAudioPlay timed out, retrying ({i + 1}/{max_retry_count})..."
                )

        self.get_logger().error(
            f"StopAudioPlay failed after {max_retry_count} retries"
        )
        return False


def main(args=None):
    rclpy.init(args=args)
    node = StopAudioPlayClient()

    try:
        success = node.send_request()
        return 0 if success else 1
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    raise SystemExit(main())
