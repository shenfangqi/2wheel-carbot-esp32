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
