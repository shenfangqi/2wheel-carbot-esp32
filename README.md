# Carbot ESP32-S3 固件

Carbot 是基于 ESP-IDF 5.4.4 的 ESP32-S3 履带式差速小车固件，包含左右履带速度闭环、差速/滑移转向、Wi-Fi 网页控制、micro-ROS `/cmd_vel`、IMU 航向修正以及电池低压保护。舵机相关代码和接口属于旧底盘兼容项，不再参与当前底盘的运动学转向。

## 目录

- [安全须知](#安全须知)
- [环境与编译](#环境与编译)
- [烧录与串口监控](#烧录与串口监控)
- [首次启动与设备配置](#首次启动与设备配置)
- [网页控制与遥测](#网页控制与遥测)
- [micro-ROS 启动与验证](#micro-ros-启动与验证)
- [调试与故障定位](#调试与故障定位)
- [开发说明](#开发说明)

## 安全须知

- 首次烧录、修改电机、差速参数或 PID 后，把履带架空再测试。
- 保证可以立即断开电机电源，不要把网页或 ROS 停车当成唯一急停手段。
- 运动测试前确认履带、传动件和线束不会卡住。
- 启动时持续蜂鸣且 LED 快速闪烁表示电池低压告警。先用万用表检查电池和采样电路，不要直接绕过保护。
- 串口 `show` 会明文显示 Wi-Fi 密码，不要把输出粘贴到公开日志或提交中。

## 环境与编译

当前开发机约定：

- 项目：`~/Code/carbot`
- ESP-IDF：`~/esp/esp-idf-v5.4.4`
- 芯片：ESP32-S3
- 串口速率：115200

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

菜单位于 `Carbot micro-ROS settings`。当前默认值为：启用 micro-ROS、Domain ID 0、`cmd_vel` 超时 500 ms、executor 周期 50 ms。

## 烧录与串口监控

连接可传数据的 USB 线，插拔前后比较端口：

```bash
ls /dev/cu.*
```

将示例端口替换为实际端口：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

固件已烧录时仅打开监控：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 monitor
```

按 `Ctrl+]` 退出 `idf.py monitor`。若端口被占用，先关闭其他串口工具或 monitor。

## 首次启动与设备配置

健康电池启动时会短鸣一次，串口输出类似：

```text
=== CARBOT START ===
battery voltage at startup: 7.40V
config loaded
config init ok
cli start ok
servo init ok
imu init start
differential controller init ok
command mux init ok
motor pid init ok
telemetry buffer init ok
wifi manager init ok
ros executor start
ros executor init ok
imu init start
uart cli ready
carbot>
```

实际电压、NVS 和网络状态会不同。固件把全局 ESP 日志设为 `ERROR`，因此许多 `ESP_LOGI/W` 默认不可见；上述 `printf` 启动节点仍会显示。

CLI 与启动日志共用 UART0：

```text
show
set wifi_ssid <SSID>
set wifi_password <PASSWORD>
set agent_ip <Agent主机IPv4>
set agent_port <端口>
save
reboot
```

首次配置示例：

```text
set wifi_ssid MyHotspot
set wifi_password 12345678
set agent_ip 192.168.1.100
set agent_port 8888
save
reboot
```

注意：

- `set` 只改内存；必须 `save` 后才写入 NVS。
- Wi-Fi 和 Agent 参数在启动时使用，所以保存后要重启。
- 当前解析器不支持含空格的 SSID 或密码。
- 使用 2.4 GHz、WPA2 兼容网络；ESP32-S3 不能连接仅 5 GHz 热点。
- `show` 中 `local_ip` 为空说明还没有获得 DHCP 地址。

## 网页控制与遥测

设备获得 IP 后，在同一网络的浏览器打开：

```text
http://<local_ip>/
```

网页提供前进、后退、左右差速转向、停止、遗留舵机中心偏置和遥测入口。手动控制当前使用固定线速度 `0.40 m/s`、角速度 `2.0 rad/s`；左右转向由两侧履带速度差实现，舵机不是转向源。

HTTP 接口：

| 地址 | 功能 |
|---|---|
| `/` | 控制页面 |
| `/cmd?move=forward`、`backward`、`left`、`right`、`center`、`stop` | 控制命令 |
| `/cmd?move=center_offset_inc`、`center_offset_dec` | 遗留舵机中心每次调整 1°并写入 NVS；不影响差速转向 |
| `/servo/offset` | 遗留舵机中心偏置；不作为运动学输入 |
| `/telemetry` | CSV 遥测 |
| `/telemetry/reset` | 清空遥测缓存 |

无运动风险的检查：

```bash
curl "http://<local_ip>/servo/offset"
curl "http://<local_ip>/telemetry" -o carbot-telemetry.csv
```

不要用自动化脚本随意请求运动接口；命令收到后会立即驱动车辆。

遥测 CSV 包含 M1/M3 目标 RPM、实际 RPM、PWM、陀螺仪 Z 轴和 IMU 状态。调试速度闭环时先看：

- 有目标、实际始终为 0：检查编码器、机械卡滞和方向映射。
- PWM 很大但实际速度很低：检查电池、负载、死区、接线或堵转。
- 左右目标符号不同是当前底盘前进方向映射的正常设计。

## micro-ROS 启动与验证

当前约定：

- Host：Jetson，ROS 2 Humble
- Transport：Wi-Fi + UDP
- Agent 默认端口：8888
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
变化时必须重置增量基准。

固件会拒绝非有限值以及超过 `0.5 m/s`、`3.5 rad/s` 的命令，并在 20 ms 控制循环
中限制线加速度为 `0.5 m/s²`、角加速度为 `2.5 rad/s²`。这些限制独立于 Jetson
上的 velocity smoother。

仓库不负责安装 Jetson 的 ROS 2 与 micro-ROS Agent。已安装的 Host 上执行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888 -v6
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

固件已有连接重试和资源清理逻辑，但 Device 先启动、Agent 重启、Wi-Fi 恢复等场景仍应按 [TODO_MICRO_ROS_RECONNECT.md](TODO_MICRO_ROS_RECONNECT.md) 做实机验收，不能仅凭编译通过视为完成。

## 调试与故障定位

按层排查，不要一开始同时修改多个模块：

1. **编译**：确认 `IDF_PATH` 是 5.4.4，从第一条编译错误开始处理。
2. **烧录**：确认真实 `/dev/cu.*`、关闭其他串口工具、重新插拔 USB。
3. **启动**：从 `=== CARBOT START ===` 开始保存完整日志，确定最后一个成功节点。
4. **电池**：持续蜂鸣/闪灯时用万用表对照串口电压，检查 GPIO3 分压采样。
5. **CLI**：确认 UART0、115200，按 Enter 查看提示符。
6. **Wi-Fi**：用 `show` 检查配置和 `local_ip`，确认 2.4 GHz、密码、DHCP 和同网段。
7. **网页**：先请求无运动风险的 `/servo/offset`，再在架空车轮条件下测控制。
8. **Agent**：检查 UDP 8888、主机防火墙/路由、Agent 输出和 Domain ID。
9. **ROS**：检查 node、topic、消息类型；记住单条消息 500 ms 后超时是预期行为。
10. **运动控制**：导出 `/telemetry`，对照目标 RPM、实际 RPM、PWM、方向和 IMU。

需要看到 `ESP_LOGI/W` 时，可暂时把 `app_main.c` 中全局日志等级从 `ESP_LOG_ERROR` 提高，定位后再决定是否保留。不要把增加日志与实际修复混为一谈。

每次硬件相关改动至少验证相关场景：健康启动、停车、前进/倒退、转向、Web/ROS 切换、通信丢失、低压保护。明确记录哪些只编译通过，哪些完成架空测试或地面测试。

## 开发说明

主要模块：

| 目录 | 职责 |
|---|---|
| `main/drivers` | GPIO、PWM、PCNT、ADC 等硬件访问 |
| `main/control` | 差速/滑移转向、PID、IMU 航向修正、命令仲裁与安全；舵机代码仅作遗留兼容 |
| `main/network` | Wi-Fi、HTTP、micro-ROS UDP 地址 |
| `main/ros_interface` | ROS node、topic、callback、生命周期和超时 |
| `main/app_config` | 默认配置、NVS 和串口 CLI |

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
