# ESP32 micro-ROS 实机验收

## 安全准备

- 首轮运动测试必须架空履带。
- 确认可以立即切断电机电源；Jetson 页面停止和 ROS watchdog 不是物理急停。
- Jetson 已构建并 source `extra_ros_packages/carbot_msgs`。

## Topic 与数据

1. 启动 Agent 后确认 `/cmd_vel`、`/wheel_ticks`、`/imu/data_raw`、
   `/battery_state` 和 `/carbot/status` 存在。
2. 静止时观察 30 秒：轮计数不应持续漂移；`sequence` 单调增加；同一次启动的
   `boot_id` 保持不变；`device_stamp_us` 严格递增。
3. 分别低速正转和反转左右履带，确认左右计数的车辆前进方向均为正。
4. 对比 IMU 静止重力方向和手动绕三轴旋转方向，确认 `imu_link` 到 `base_link`
   的安装轴向；当前固件按传感器 XYZ 直接发布，未通过本测试前不得用于融合。
5. 用万用表对比 `/battery_state.voltage`，并验证未测量字段为 NaN。

## 命令安全

1. 连续发送低速 `/cmd_vel`，确认速度按斜率上升而非阶跃。
2. 发送 NaN、Inf、超过 0.5 m/s 或超过 3.5 rad/s 的命令，确认拒绝计数增加且
   ROS 已拥有运动时立即停车。
3. 停止发布，确认约 500 ms 后停车；发送零 Twist 也应停车。
4. 确认 `/carbot/status.active_command_source` 仅报告 `NONE=0` 或 `ROS=2`。

## 连接与时间

1. ESP32 先启动、Agent 后启动，确认无需重启 ESP32 即可出现 node 和 topics。
2. 运动中关闭 Agent，确认 ROS 所属运动立即停止；重启 Agent 后实体自动重建。
3. 断开并恢复 Wi-Fi，确认自动重建且 `/carbot/status.reconnect_count` 增加。
4. 每次重连确认 `time_synchronized=true`；对比消息 `header.stamp` 与 Jetson 当前
   ROS 时间，并记录测试期间最大偏差。
5. ESP32 重启后确认 `boot_id` 改变，Jetson 重置轮计数增量基准且不产生里程计跳变。

## 通过条件

- 上述测试全部记录实际结果；任何仅由编译或 host 单元测试覆盖的项目不能标记为
  实机通过。
- 低压保护、启动停车、前进/后退、原地转向和 ROS 命令控制均无回归。
- ESP32 的 TCP 80 未监听，且不存在 HTTP 运动 endpoint。
