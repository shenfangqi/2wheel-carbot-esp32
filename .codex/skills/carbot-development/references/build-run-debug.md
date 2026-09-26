# Build, run, and debug reference

## Contents

- Build and flash
- First boot and CLI
- Motion control and telemetry
- micro-ROS host workflow
- Troubleshooting ladder
- Verification matrix

## Build and flash

Host assumptions: macOS project path `/Users/shenfangqi/Code/carbot`, ESP-IDF 5.4.4 installed at `/Users/shenfangqi/esp/esp-idf-v5.4.4`. Adapt absolute paths only when the workstation differs.

```bash
cd /Users/shenfangqi/Code/carbot
source /Users/shenfangqi/esp/esp-idf-v5.4.4/export.sh
echo "$IDF_PATH"
idf.py build
```

Use `idf.py fullclean` only for stale configuration/dependency symptoms; normal incremental builds should use `idf.py build`.

Find the port by comparing `ls /dev/cu.*` before and after connecting the board. Then:

```bash
idf.py -p /dev/cu.usbserial-0001 -b 460800 flash
```

Stop the Jetson micro-ROS Agent before flashing so it does not hold the CP2102 port. Do not attach `idf.py monitor` to production firmware: UART0 carries binary XRCE frames at 921600. The checked-in `sdkconfig` already targets ESP32-S3; use `idf.py menuconfig` for Kconfig changes and review `sdkconfig` diffs before committing.

## First boot and console policy

Keep the drive wheels raised for initial bring-up. Healthy startup begins with a short beep. A continuous buzzer plus fast LED blinking means low-voltage alarm; do not bypass it without measuring the battery/ADC circuit. Production firmware disables the ESP console, text logs, and CLI so UART0 is exclusively available to framed micro-ROS traffic. Use a separate USB Serial/JTAG connection and a dedicated debug configuration if board-level text diagnostics are needed.

## Motion control and telemetry

The ESP32 does not run an HTTP server; TCP port 80 should be closed. Manual control is hosted
on Jetson and must be converted to a continuous `/cmd_vel` stream. The firmware keeps its
internal telemetry buffer and publishes wheel ticks, IMU, battery, and status over ROS.

## micro-ROS host workflow

Firmware defaults: CP2102/UART0 custom framed transport at 921600 8N1, ROS domain 0, node `/carbot_base`, subscription `/cmd_vel`, `geometry_msgs/msg/Twist`, 500 ms command watchdog.

The host is expected to provide ROS 2 Humble and `micro_ros_agent`. Installation is outside this repository. On a configured Jetson:

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
ros2 run micro_ros_agent micro_ros_agent serial \
  --dev /dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0 \
  --baudrate 921600 -v6
```

Start the Agent before rebooting the ESP32 for the most reliable current workflow. After connection:

```bash
ros2 node list
ros2 topic list
ros2 topic info /cmd_vel -v
```

Raised-wheels command test, automatically stopped by the 500 ms firmware watchdog:

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.10}, angular: {z: 0.0}}"
```

Continuous driving requires messages faster than 2 Hz; use a conservative rate such as 10 Hz and stop with Ctrl+C, then publish a zero command:

```bash
ros2 topic pub -r 10 /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.10}, angular: {z: 0.0}}"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0}, angular: {z: 0.0}}"
```

The firmware retry/cleanup loop retries support initialization and tears down on transport, executor, publisher, or Agent health failure. Late Agent startup, restart, and USB reconnect remain hardware acceptance items until the scenarios in `TODO_MICRO_ROS_RECONNECT.md` pass.

## Troubleshooting ladder

1. Build: confirm `IDF_PATH`, ESP-IDF 5.4.4, and the first compiler error. Avoid diagnosing cascaded errors first.
2. Flash: confirm the actual `/dev/cu.*` port, close other monitors, reconnect USB, and retry.
3. Boot: check the startup beep and battery alarm indications; production UART0 does not emit text logs.
4. Serial: verify the stable CP2102 by-id path, 921600 baud, permissions, and that no monitor holds the port.
5. Agent: verify serial Agent output and `ROS_DOMAIN_ID=0`.
6. ROS: inspect node/topic/type and Agent output. A single command expires after 500 ms by design.
7. Motion: compare telemetry target, actual RPM, and PWM. Target with zero actual suggests encoder/mechanical trouble; large PWM with low actual suggests load, dead zone, battery, wiring, or stall.
8. Logs: use the board's separate USB Serial/JTAG path and a dedicated debug configuration; never mix text with UART0 XRCE traffic.

## Verification matrix

Select relevant rows for each change and record actual evidence:

| Scenario | Expected result |
|---|---|
| Build | `idf.py build` succeeds and partition size remains valid |
| Healthy boot | Short beep, no alarm loop, initialization milestones complete |
| Low battery | Motor blocked/braked, servo centered, buzzer on, LED blinking |
| Agent absent | No motion starts; firmware keeps retrying the serial session |
| Jetson manual control | Publishes `/cmd_vel` at 10 Hz; release/stop publishes zero or lets watchdog expire |
| ROS single message | Motion begins and stops/centers after about 500 ms |
| Agent/USB loss | ROS-owned motion stops and centers |
| ROS telemetry | Wheel, IMU, battery, and status topics remain available |

Do not claim Agent recovery, battery cutoff accuracy, track calibration, or PID stability without real hardware observations.
