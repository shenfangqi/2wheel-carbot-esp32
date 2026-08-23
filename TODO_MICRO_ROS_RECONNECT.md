# TODO: micro-ROS 重连与 Host 晚启动支持

## 背景

当前 ESP32 端已经完成了第一阶段 micro-ROS 接入：

- Jetson 通过 `ROS 2 Humble + micro-ROS Agent`
- ESP32 通过 Wi-Fi / UDP 接入
- 板子端订阅 `cmd_vel`
- 收到 `geometry_msgs/msg/Twist` 后驱动 Ackermann 控制层

当前现状：

- 如果 **host 先启动**，再启动 device，`/cmd_vel` 可以正常工作
- 如果 **device 先启动**，host 后启动，当前版本里从 host 发送 `cmd_vel` **没有效果**

这说明当前实现虽然有基础重试循环，但还没有做到稳定的：

- Host 晚启动后自动接入
- Agent 掉线后自动恢复

## 目标

后续需要把 ESP32 端补成下面这种行为：

1. device 先启动，host 后启动时，ESP32 能自动连接到 Agent
2. Agent 运行中断开后，ESP32 能自动检测并回到等待重连状态
3. Agent 恢复后，ESP32 能重新建立 ROS node / subscriber
4. 恢复后继续接收 `/cmd_vel`
5. 掉线期间小车必须安全停车并回正

## 需要改进的方向

### 1. Agent 可用性探测

在创建 ROS node / executor / subscriber 之前，先做 Agent 可用性确认。

建议方向：

- 在初始化 ROS 实体前先 `ping agent`
- 只有确认 Agent 在线后才继续 `rclc_support_init_with_options(...)`

目的：

- 避免 device 先启动时卡在不完整状态
- 让 ROS 初始化过程和 Agent 真正在线状态绑定

### 2. 运行期掉线检测

当前 executor 循环里只基于：

- Wi-Fi 是否连接
- `spin_some()` 是否报错

后续要补成更明确的 Agent 存活检测。

建议方向：

- 周期性 `ping agent`
- 或基于更明确的 RMW / XRCE 错误判断掉线

目的：

- 区分“Wi-Fi 还在，但 Agent 已经没了”的情况

### 3. 掉线后的资源销毁与状态回退

当前已有基本 cleanup：

- `ros_publishers_fini(...)`
- `ros_subscribers_fini(...)`
- `rclc_executor_fini(...)`
- `rcl_node_fini(...)`
- `rclc_support_fini(...)`

但后续要明确它对应的状态机语义：

- 已连接
- 掉线
- 清理完成
- 等待重连
- 重新初始化

目的：

- 避免“看起来任务还在跑，但其实已经无法重新接管”的半失效状态

### 4. 掉线期间安全停车

当前已经有：

- `cmd_vel timeout` 停车
- ROS stop 只影响 ROS source

后续需要明确保证：

- Agent 掉线时一定执行 `command_mux_stop_ros(true)`
- 不影响网页手动控制逻辑

目的：

- ROS 失联时车必须安全
- 同时保留网页控制独立性

### 5. Host 晚启动联调验证

后续补完后，需要专门验证这组场景：

1. Jetson 先启动，ESP32 后启动
2. ESP32 先启动，Jetson 后启动
3. 运行中关闭 Agent，再重启 Agent
4. 运行中断开 Wi-Fi，再恢复 Wi-Fi
5. 网页控制正在使用时，ROS 连接恢复

## 验收标准

满足以下条件才算这个 TODO 完成：

- ESP32 先启动时，Jetson 后启动 Agent，板子能自动恢复并接收 `/cmd_vel`
- Agent 中途退出后，小车自动停车并回正
- Agent 恢复后，不重启板子也能再次接收 `/cmd_vel`
- 网页控制逻辑不被 ROS 重连机制破坏
- 编译通过，固件可刷写，实机联调通过

## 备注

当前建议的临时使用方式仍然是：

1. 先启动 host（Jetson）
2. 再启动 device（ESP32）

这是当前版本的已知限制，不是最终目标行为。
