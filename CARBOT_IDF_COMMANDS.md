# Carbot ESP-IDF 常用命令

> 本项目统一使用 `source ~/esp/esp-idf-v5.4.4/export.sh`
>
> 不要使用 `source ~/esp/esp-idf/export.sh`

## 环境准备

```bash
cd ~/Code/carbot
ls /dev/cu.*
source ~/esp/esp-idf-v5.4.4/export.sh
```

## 清理并编译

```bash
idf.py fullclean
idf.py build
```

## 烧录并监控

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

## micro-ROS 启动顺序

当前项目的 micro-ROS 第一阶段联调，要求 **host 先启动，再启动 device**。

- `host`
  - 含义：Jetson 上的 `ROS 2 Humble + micro-ROS Agent`
- `device`
  - 含义：ESP32 小车控制板

当前已知限制：

- 如果 `host` 先启动，再启动 `device`，`/cmd_vel` 可以正常工作
- 如果 `device` 先启动，`host` 后启动，当前版本里从 `host` 发送 `/cmd_vel` 可能没有效果

当前推荐联调顺序：

1. 在 Jetson 上先启动 `ROS 2 Humble`
2. 在 Jetson 上启动 `micro-ROS Agent`
3. 再给 ESP32 上电
4. 等 ESP32 连上 Wi-Fi 和 Agent 后，再从 host 发送 `/cmd_vel`

Jetson 端 `micro-ROS Agent` 启动命令占位：

```bash
# TODO: 在 Jetson 上确认最终命令后回填
# 示例占位：
# ros2 run micro_ros_agent micro_ros_agent udp4 --port <PORT>
```

## 仅打开监控

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 monitor
```

## 常用串口查看

查看当前可用串口：

```bash
ls /dev/cu.*
```

确认目标串口是否存在：

```bash
ls /dev/cu.usbserial-0001
```

重新插拔开发板前后，对比串口列表：

```bash
ls /dev/cu.*
```

## 首次打开项目的 VS Code 注意事项

- 打开项目根目录 `~/Code/carbot`，不要只打开 `main/` 目录。
- 使用 ESP-IDF `v5.4.4`：

```bash
source ~/esp/esp-idf-v5.4.4/export.sh
```

- 如果 VS Code 里 IntelliSense / Problems 出现很多假报错，确认它使用的是根目录构建产物：

```bash
~/Code/carbot/build/compile_commands.json
```

- 切换 ESP-IDF 环境或重新编译后，建议重启 VS Code，或者执行 `C/C++: Reset IntelliSense Database`。

## 常见故障排查命令

重新加载 ESP-IDF 环境：

```bash
source ~/esp/esp-idf-v5.4.4/export.sh
```

查看当前使用的 IDF 路径：

```bash
echo $IDF_PATH
```

清理并重新编译工程：

```bash
idf.py fullclean
idf.py build
```

如果监控卡住，重新插拔开发板后再打开监控：

```bash
idf.py -p /dev/cu.usbserial-0001 -b 115200 monitor
```

如果烧录失败，先重新确认串口是否存在：

```bash
ls /dev/cu.*
```
