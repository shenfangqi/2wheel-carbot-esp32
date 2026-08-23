# Carbot ESP32-S3 使用说明

本项目是一个基于 `ESP-IDF v5.4.4` 的 ESP32-S3 小车控制工程。

如果你只是想尽快上手，建议按下面顺序操作：

1. 打开一个新终端，进入项目目录并加载 ESP-IDF 环境
2. 连接开发板，确认串口
3. 烧录或直接打开串口监控
4. 进入串口 CLI，配置手机热点和 `micro-ROS Agent`
5. 重启开发板，确认 Wi-Fi 连通和启动日志正常

## 1. 环境准备

每次新开一个终端，都先执行：

```bash
cd ~/Code/carbot
source ~/esp/esp-idf-v5.4.4/export.sh
```

不要使用：

```bash
source ~/esp/esp-idf/export.sh
```

确认当前加载的是正确的 IDF：

```bash
echo $IDF_PATH
```

## 2. 连接开发板并确认串口

用可传数据的 USB 线连接 ESP32-S3 开发板和 Mac。

查看当前串口：

```bash
ls /dev/cu.*
```

常见串口名示例：

- `/dev/cu.usbserial-0001`
- `/dev/cu.usbmodem*`

如果不确定哪个是开发板串口，可以在插拔开发板前后各执行一次：

```bash
ls /dev/cu.*
```

## 3. 编译、烧录、查看启动日志

### 清理并编译

```bash
idf.py fullclean
idf.py build
```

### 烧录并打开串口监控

把下面命令里的串口替换成你机器上的实际串口：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

### 只打开串口监控

如果固件已经烧录过，只想看启动日志：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 monitor
```

### 退出串口监控

退出 `idf.py monitor`：

```text
Ctrl+]
```

## 4. 启动后你会看到什么

正常启动时，串口里通常会看到类似输出：

```text
=== CARBOT START ===
config init ok
cli start ok
servo init ok
ackermann controller init ok
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

说明：

- `carbot>` 就是串口 CLI 提示符
- 启动日志和 CLI 共用同一个串口
- 如果没开热点或 Wi-Fi 配置不对，可能会看到 `wifi_manager: connect timeout` 或 `connect to the AP failed`

## 5. 进入 CLI 并配置 Wi-Fi

看到下面提示符后，就可以直接输入命令：

```text
carbot>
```

当前固件支持的常用命令：

- `show`
- `set wifi_ssid <SSID>`
- `set wifi_password <PASSWORD>`
- `set agent_ip <IP>`
- `set agent_port <PORT>`
- `save`
- `reboot`

建议第一次先配置手机热点。

### 查看当前配置

```text
show
```

### 配置手机热点

示例：

```text
set wifi_ssid MyHotspot
set wifi_password 12345678
save
reboot
```

注意：

- 手机热点建议使用 `2.4GHz`
- `ESP32-S3` 不能连接仅 `5GHz` 的热点
- 如果热点名或密码里带空格，当前 CLI 不适合直接配置，建议先改成不含空格的热点名/密码

## 6. 配置 micro-ROS Agent

本项目当前的 `micro-ROS` 使用的是 `Wi-Fi + UDP`，不是串口 transport。

如果你还要让 ESP 连接 `micro-ROS Agent`，需要配置 Agent 所在主机的 IP 和端口。

示例：

```text
set agent_ip 192.168.1.100
set agent_port 8888
save
reboot
```

如果只想看板子启动和本地日志，不跑 ROS，`agent_ip` 先不配也可以。

## 7. 推荐启动顺序

当前项目联调时，推荐顺序是：

1. 先打开手机热点或其他 2.4GHz Wi-Fi
2. 如果要跑 ROS，先在 Host 端启动 `ROS 2` 和 `micro-ROS Agent`
3. 再给 ESP32 上电或重启
4. 打开串口监控，观察启动日志

## 8. 常见操作流程

### 流程 A：只看启动日志

```bash
cd ~/Code/carbot
source ~/esp/esp-idf-v5.4.4/export.sh
idf.py -p /dev/cu.usbserial-0001 -b 115200 monitor
```

### 流程 B：重新烧录最新固件

```bash
cd ~/Code/carbot
source ~/esp/esp-idf-v5.4.4/export.sh
idf.py fullclean
idf.py build
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

### 流程 C：首次配置热点和 Agent

1. 烧录并进入 `monitor`
2. 等待出现 `carbot>`
3. 输入：

```text
set wifi_ssid MyHotspot
set wifi_password 12345678
set agent_ip 192.168.1.100
set agent_port 8888
save
reboot
```

## 9. 常见问题

### `wifi_manager: connect timeout`

常见原因：

- 手机热点没打开
- 热点不是 `2.4GHz`
- `SSID` 或密码配置错误
- 设备里保存的是旧配置

### `show` 命令不存在

如果 `reboot` 能用，但 `show` 不存在，通常说明开发板里跑的不是当前仓库这版固件。先重新烧录：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

### 串口打不开

先确认串口存在：

```bash
ls /dev/cu.*
```

再确认是否使用了正确端口。

### 切换终端后 `idf.py` 不能用

通常是忘了重新加载环境：

```bash
source ~/esp/esp-idf-v5.4.4/export.sh
```

## 10. 相关文档

- [CARBOT_IDF_COMMANDS.md](CARBOT_IDF_COMMANDS.md)
- [ROS_ACKERMANN_UNITS_EXAMPLE.md](ROS_ACKERMANN_UNITS_EXAMPLE.md)
