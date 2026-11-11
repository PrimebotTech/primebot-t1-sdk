#!/usr/bin/env python3

"""
系统重启示例脚本

Description:
  通过 MigrateSystemState 服务请求机器人执行系统状态迁移（如重启）。
  该接口在系统管理层实现，通过状态机机制完成安全的状态切换。

  警告：调用此接口后，系统将进入安全的重启流程，依次停止各子系统服务后执行重启。
  请确保在调用前已保存重要数据。

  ⚠ 摔机风险警告：本接口不会自动切换机器人动作状态。执行前务必先通过上层
  业务逻辑将机器人切换至趴下（Prone）或零力矩等安全姿态，否则重启过程中
  关节失去力矩支撑，机器人可能直接摔倒造成硬件损坏！

Prerequisites:
  - 系统状态管理服务必须正在运行
  - 机器人在安全状态下可执行重启
  - ⚠ 机器人已切换至趴下或零力矩等安全姿态（本接口不会自动切换动作）

Usage:
  python3 migrate_system_state.py

Example:
  python3 migrate_system_state.py

Parameters:
  - state: 目标系统状态（如 "Reboot"）
"""

import time

import rclpy
import rclpy.logging
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.node import Node

from aimdk_msgs.msg import CommonRequest
from aimdk_msgs.srv import MigrateSystemState

SERVICE_WAIT_SEC = 2.0
SERVICE_CALL_TIMEOUT_SEC = 5.0
MAX_RETRY_COUNT = 3

SERVICE_NAME = "/aimdk_5Fmsgs/srv/MigrateSystemState"


class MigrateSystemStateClient(Node):
    """MigrateSystemState service client node."""

    def __init__(self):
        super().__init__("migrate_system_state_client")

        self.cb_group = ReentrantCallbackGroup()

        self.client = self.create_client(
            MigrateSystemState,
            SERVICE_NAME,
            callback_group=self.cb_group,
        )

        self.get_logger().info("MigrateSystemState client node created.")

        while not self.client.wait_for_service(timeout_sec=SERVICE_WAIT_SEC):
            if not rclpy.ok():
                return
            self.get_logger().info("Waiting for service...")
        self.get_logger().info("Service available, ready to send request.")

    def send_request(self, state: str = "Reboot") -> bool:
        """
        Send MigrateSystemState request.

        Args:
            state: Target system state (e.g., "Reboot").

        Returns:
            True if request was successful, False otherwise.
        """
        request = MigrateSystemState.Request()
        request.header = CommonRequest()
        request.header.header.stamp = self.get_clock().now().to_msg()
        request.state = state

        self.get_logger().info(
            f"Sending MigrateSystemState request: state={state}"
        )

        for attempt in range(1, MAX_RETRY_COUNT + 1):
            future = self.client.call_async(request)
            rclpy.spin_until_future_complete(
                self, future, timeout_sec=SERVICE_CALL_TIMEOUT_SEC
            )

            if future.done():
                try:
                    response = future.result()
                except Exception as error:  # noqa: BLE001
                    self.get_logger().error(f"MigrateSystemState failed: {error}")
                    return False

                if response is None:
                    self.get_logger().error("MigrateSystemState returned no response.")
                    return False

                code = response.header.header.code
                if code == 0:
                    self.get_logger().info(
                        f"MigrateSystemState request successful. "
                        f"Robot is transitioning to state: {state}..."
                    )
                    return True

                self.get_logger().error(
                    f"MigrateSystemState failed with code: {code}, "
                    f"retrying ({attempt}/{MAX_RETRY_COUNT})..."
                )
            else:
                self.get_logger().warning(
                    f"MigrateSystemState timed out, retrying ({attempt}/{MAX_RETRY_COUNT})..."
                )

            if attempt < MAX_RETRY_COUNT:
                time.sleep(1.0)

        self.get_logger().error(
            f"MigrateSystemState failed after {MAX_RETRY_COUNT} retries"
        )
        return False


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        print("Select system operation:")
        print("  1. Reboot (重启)")
        print("  2. Shutdown (关机)")
        choice = input("Enter choice (default 1): ").strip()

        if choice == "2":
            state = "Shutdown"
        else:
            state = "Reboot"

        node = MigrateSystemStateClient()
        ok = node.send_request(state)
        return 0 if ok else 1
    except Exception as error:  # noqa: BLE001
        rclpy.logging.get_logger("main").error(
            f"Program exited with exception: {error}"
        )
        return 1
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
if __name__ == "__main__":
    raise SystemExit(main())
