# Carbot ESP32-S3 固件

Carbot 是基于 ESP-IDF 5.4.4 的 ESP32-S3 履带式差速小车固件，包含左右履带速度闭环、差速/滑移转向、USB-UART micro-ROS `/cmd_vel`、IMU 航向修正以及电池低压保护。舵机相关代码属于旧底盘兼容项，不再参与当前底盘的运动学转向。

## 目录

- [安全须知](#安全须知)
- [环境与编译](#环境与编译)
- [烧录与串口使用](#烧录与串口使用)
- [首次启动与设备配置](#首次启动与设备配置)
- [运动控制与遥测](#运动控制与遥测)
- [micro-ROS 启动与验证](#micro-ros-启动与验证)
- [调试与故障定位](#调试与故障定位)
- [开发说明](#开发说明)

## 安全须知

- 首次烧录、修改电机、差速参数或 PID 后，把履带架空再测试。
- 保证可以立即断开电机电源，不要把 Jetson 页面或 ROS 停车当成物理急停。
- 运动测试前确认履带、传动件和线束不会卡住。
- 启动时持续蜂鸣且 LED 快速闪烁表示电池低压告警。先用万用表检查电池和采样电路，不要直接绕过保护。
- 生产固件不提供 UART0 CLI；不要用串口终端向 XRCE 链路写入文本。

## 环境与编译

当前开发机约定：

- 项目：`~/Code/carbot`
- ESP-IDF：`~/esp/esp-idf-v5.4.4`
- 芯片：ESP32-S3
- micro-ROS 串口速率：921600，8N1，无硬件流控

每个新终端都先加载环境：

```bash
cd ~/Code/carbot
source ~/esp/esp-idf-v5.4.4/export.sh
echo "$IDF_PATH"
```

正常增量编译：

```bash
idf.py build
```

只有在切换 IDF、配置或依赖异常时才清理重编：

```bash
idf.py fullclean
idf.py build
```

成功时末尾会显示 `Project build complete`、固件大小和剩余分区空间。项目依赖由 ESP-IDF Component Manager 和仓库内组件提供；首次构建可能比增量构建慢。

修改 micro-ROS Kconfig 参数时：

```bash
idf.py menuconfig
```

菜单位于 `Carbot micro-ROS settings`。当前默认值为：启用 micro-ROS、Domain ID 0、`cmd_vel` 超时 500 ms、executor 周期 50 ms。运行期 Agent 健康检查每 5 秒执行一次，每轮最多进行 2 次 150 ms 尝试，连续 3 轮失败才重建会话。

## 烧录与串口使用

连接可传数据的 USB 线，插拔前后比较端口：

```bash
ls /dev/cu.*
```

烧录前先停止 Jetson 上的 micro-ROS Agent，避免它占用 CP2102。将示例端口替换为实际端口：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 460800 flash
```

不要在生产固件上运行 `idf.py monitor`：UART0 由 micro-ROS 独占，其中是二进制 XRCE framing，不是文本日志。

## 首次启动与设备配置

健康电池启动时会短鸣一次。生产配置关闭 ESP console、ESP_LOG、`printf` 启动输出和 UART CLI，UART0 只用于 micro-ROS。Wi-Fi SSID、密码、Agent IP/端口不再加载或写入 NVS；仅保留旧舵机中心偏移配置。已有 NVS 中的旧网络 key 会被忽略，无需擦除。

## 运动控制与遥测

ESP32 不提供 HTTP 服务或运动 API。手机手动操作由 Jetson 页面转换为持续发布的
`/cmd_vel`，与导航命令一样经 micro-ROS 进入固件。Jetson 应以高于 2 Hz 的频率持续
发布，建议 10 Hz；停止发布后 ESP32 的 500 ms watchdog 会停车。Jetson 页面 Stop
不是物理急停。

内部 telemetry buffer 仍用于固件诊断，但不再通过 HTTP 导出。运行期遥测由 ROS
topics `/wheel_ticks`、`/imu/data_raw`、`/battery_state` 和 `/carbot/status` 发布。
整车电源关闭但 USB 仍在供电时，电池 ADC 会显示低电压/电池断开。
固件会立即清除命令归属、刹停并禁止运动，但不会进入永久深度睡眠；
UART/micro-ROS 保持运行，以便 `/battery_state` 和 `/carbot/status` 报告故障。
电压恢复到 6.90 V 以上后解除阻止，但必须收到新的有效 `/cmd_vel` 才会运动。
调试速度闭环时可结合串口诊断和 ROS 遥测检查：

- 有目标、实际始终为 0：检查编码器、机械卡滞和方向映射。
- PWM 很大但实际速度很低：检查电池、负载、死区、接线或堵转。
- 左右目标符号不同是当前底盘前进方向映射的正常设计。

## micro-ROS 启动与验证

当前约定：

- Host：Jetson，ROS 2 Humble
- Transport：CP2102 USB-UART，UART0（TX GPIO43 / RX GPIO44），921600 8N1
- ROS Domain ID：0
- Node：`/carbot_base`
- Subscriber：`/cmd_vel`，类型 `geometry_msgs/msg/Twist`
- Publisher：`/wheel_ticks`，类型 `carbot_msgs/msg/WheelTicks`，50 Hz
- Publisher：`/imu/data_raw`，类型 `sensor_msgs/msg/Imu`，50 Hz
- Publisher：`/battery_state`，类型 `sensor_msgs/msg/BatteryState`，2 Hz
- Publisher：`/carbot/status`，类型 `carbot_msgs/msg/CarbotStatus`，2 Hz
- 命令看门狗：500 ms

`carbot_msgs` 的接口源码位于 `extra_ros_packages/carbot_msgs`。Jetson 必须在自己的
ROS 2 workspace 中复制或引用该包并执行 `colcon build`，否则无法解析
`/wheel_ticks` 和 `/carbot/status`。轮计数消息同时包含 64 位左右累计计数、
`sequence`、`boot_id` 和 ESP32 单调时钟 `device_stamp_us`。Jetson 发现 `boot_id`
变化时必须重置增量基准。状态消息还包含最后一次断开原因、连续 Agent ping
失败次数和当前/上一次 session 存活时间；修改消息定义后必须在 Jetson workspace
中重新构建 `carbot_msgs`。本次将数值 1 的断开原因从 Wi-Fi 专用名改为
`DISCONNECT_TRANSPORT`，数值未变，但 Jetson 仍需重建消息包以获得新常量名。

固件会拒绝非有限值以及超过 `0.5 m/s`、`3.5 rad/s` 的命令，并在 20 ms 控制循环
中限制线加速度为 `0.5 m/s²`、角加速度为 `2.5 rad/s²`。这些限制独立于 Jetson
上的 velocity smoother。

仓库不负责安装 Jetson 的 ROS 2 与 micro-ROS Agent。已安装的 Host 上执行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
ros2 run micro_ros_agent micro_ros_agent serial \
  --dev /dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0 \
  --baudrate 921600 -v6
```

固件在创建 ROS 实体前探测 Agent，运行期间周期检查 Agent，并在连接重建后重新
同步 micro-ROS epoch。连接后检查：

```bash
ros2 node list
ros2 topic list
ros2 topic info /cmd_vel -v
ros2 topic echo /wheel_ticks --once
ros2 topic echo /imu/data_raw --once
ros2 topic echo /battery_state --once
ros2 topic echo /carbot/status --once
```

架空车轮后发送一次低速命令：

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.10}, angular: {z: 0.0}}"
```

单次命令会在约 500 ms 后由固件自动停车并回中。连续测试要以高于 2 Hz 发布，建议 10 Hz；停止发布后再显式发送零命令：

```bash
ros2 topic pub -r 10 /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.10}, angular: {z: 0.0}}"

ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0}, angular: {z: 0.0}}"
```

固件已有连接重试和资源清理逻辑，但 Device 先启动、Agent 重启和 USB 拔插场景仍应按 [TODO_MICRO_ROS_RECONNECT.md](TODO_MICRO_ROS_RECONNECT.md) 做实机验收，不能仅凭编译通过视为完成。

## 调试与故障定位

按层排查，不要一开始同时修改多个模块：

1. **编译**：确认 `IDF_PATH` 是 5.4.4，从第一条编译错误开始处理。
2. **烧录**：确认真实 `/dev/cu.*`、关闭其他串口工具、重新插拔 USB。
3. **串口**：确认 CP2102 by-id 路径、921600 波特率，并关闭所有 monitor。
4. **电池**：持续蜂鸣/闪灯时用万用表检查 GPIO3 分压采样。
5. **Agent**：检查 serial Agent 输出、设备权限、by-id 路径和 Domain ID。
9. **ROS**：检查 node、topic、消息类型；记住单条消息 500 ms 后超时是预期行为。
10. **运动控制**：结合 ROS topics 和串口诊断，对照目标、反馈、方向和 IMU。

如需文本诊断，应使用板级独立 USB Serial/JTAG 接口和专用调试配置；不得让日志或 CLI 与 UART0 XRCE 数据混流。

每次硬件相关改动至少验证相关场景：健康启动、停车、前进/倒退、转向、ROS 命令、通信丢失、低压保护。明确记录哪些只编译通过，哪些完成架空测试或地面测试。

## 开发说明

主要模块：

| 目录 | 职责 |
|---|---|
| `main/drivers` | GPIO、PWM、PCNT、ADC 等硬件访问 |
| `main/control` | 差速/滑移转向、PID、IMU 航向修正、命令仲裁与安全；舵机代码仅作遗留兼容 |
| `main/network` | UART0 micro-ROS custom serial transport |
| `main/ros_interface` | ROS node、topic、callback、生命周期和超时 |
| `main/app_config` | 旧舵机中心偏移的默认值与 NVS 兼容 |

后续 Codex 开发应使用仓库 skill：

```text
$carbot-development
```

详细项目上下文位于 [`.codex/skills/carbot-development/SKILL.md`](.codex/skills/carbot-development/SKILL.md)，其中包含架构、安全约束、硬件参数、调试流程和路线图。

其他资料：

- [CARBOT_IDF_COMMANDS.md](CARBOT_IDF_COMMANDS.md)：命令速查
- [ROS_DIFFERENTIAL_UNITS_EXAMPLE.md](ROS_DIFFERENTIAL_UNITS_EXAMPLE.md)：差速运动学、标定轮距和 PID 单位示例
- [TODO_MICRO_ROS_RECONNECT.md](TODO_MICRO_ROS_RECONNECT.md)：重连验收
- [TODO_MOTOR_TUNING.md](TODO_MOTOR_TUNING.md)：电机输入与死区改进
- [TODO_PID.md](TODO_PID.md)：PID 后续工作（部分描述来自旧版本，实施前对照源码）
- [TODO_STRAIGHT_LINE_AND_IMU.md](TODO_STRAIGHT_LINE_AND_IMU.md)：直线与 IMU 外环方案
