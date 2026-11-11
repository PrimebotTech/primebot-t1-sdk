#!/usr/bin/env python3

"""
外挂算力板供电控制示例脚本

Description:
  通过 SetExt12vPower 服务控制机器人 XT30 接口的外挂算力板供电状态。
  支持开启（enable: true）或关闭（enable: false）外部计算板的电源供应。

  注意：关闭供电前，请确保外挂算力板已正常关机或进入安全状态。

Prerequisites:
  - 外挂算力板供电服务必须正在运行
  - XT30 接口已正确连接外部计算板

Usage:
  python3 set_ext12v_power.py [on|off]

Example:
  python3 set_ext12v_power.py on    # 开启供电
  python3 set_ext12v_power.py off   # 关闭供电

Parameters:
  - enable: 供电使能（True=开启，False=关闭）
"""

import sys

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_services_default

from aimdk_msgs.srv import SetExt12vPower


class SetExt12vPowerClient(Node):
    def __init__(self):
        super().__init__("set_ext12v_power_client")

        self.client = self.create_client(
            SetExt12vPower,
            "/aimdk_5Fmsgs/srv/SetExt12vPower",
            qos_profile=qos_profile_services_default,
        )

        self.get_logger().info("SetExt12vPower client node created.")

        while not self.client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info("Waiting for service...")

        self.get_logger().info("Service available, ready to send request.")

    def send_request(self, enable: bool = True) -> bool:
        request = SetExt12vPower.Request()
        request.enable = enable

        self.get_logger().info(
            f"Sending SetExt12vPower request: enable={enable}"
        )

        max_retry_count = 3
        for i in range(max_retry_count):
            future = self.client.call_async(request)
            rclpy.spin_until_future_complete(self, future, timeout_sec=5.0)

            if future.done():
                response = future.result()
                if response is not None:
                    self.get_logger().info(
                        f"SetExt12vPower response code: {response.header.header.code}"
                    )
                    if response.header.header.code == 0:
                        state_str = "ON" if enable else "OFF"
                        self.get_logger().info(
                            f"SetExt12vPower request successful. "
                            f"External compute board power: {state_str}"
                        )
                        return True
                    else:
                        self.get_logger().warn(
                            f"SetExt12vPower failed with code: {response.header.header.code}, "
                            f"retrying ({i + 1}/{max_retry_count})..."
                        )
                else:
                    self.get_logger().warn(
                        f"SetExt12vPower returned None, retrying ({i + 1}/{max_retry_count})..."
                    )
            else:
                self.get_logger().warn(
                    f"SetExt12vPower timed out, retrying ({i + 1}/{max_retry_count})..."
                )

        self.get_logger().error(
            f"SetExt12vPower failed after {max_retry_count} retries"
        )
        return False


def main(args=None):
    rclpy.init(args=args)

    # 解析命令行参数
    enable = True  # 默认开启
    if sys.argv[1:]:
        arg = sys.argv[1].lower()
        if arg in ("on", "true", "1"):
            enable = True
        elif arg in ("off", "false", "0"):
            enable = False
        else:
            print(f"Usage: {sys.argv[0]} [on|off]")
            print(f"  on   : 开启外挂算力板供电 (default)")
            print(f"  off  : 关闭外挂算力板供电")
            return 1

    node = SetExt12vPowerClient()

    try:
        success = node.send_request(enable)
        return 0 if success else 1
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    raise SystemExit(main())
