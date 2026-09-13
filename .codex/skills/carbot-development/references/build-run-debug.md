# Build, run, and debug reference

## Contents

- Build and flash
- First boot and CLI
- Web control and telemetry
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
idf.py -p /dev/cu.usbserial-0001 -b 115200 flash monitor
```

Use `Ctrl+]` to exit. Use `idf.py ... monitor` without `flash` to reconnect. The checked-in `sdkconfig` already targets ESP32-S3; use `idf.py menuconfig` for Kconfig changes and review `sdkconfig` diffs before committing.

## First boot and CLI

Keep the drive wheels raised for initial bring-up. Healthy startup begins with a short beep and prints battery voltage, configuration, controller, network, ROS, and IMU milestones. A continuous buzzer plus fast LED blinking means low-voltage alarm; do not bypass it without measuring the battery/ADC circuit.

UART0 CLI commands:

```text
show
set wifi_ssid <SSID>
set wifi_password <PASSWORD>
set agent_ip <IPv4-address>
set agent_port <port>
save
reboot
```

Values cannot contain spaces. `show` prints the Wi-Fi password in clear text; do not paste its output into tickets or commits. `set` changes RAM only until `save`; networking uses values loaded at boot, so save and reboot after changes. `show` also prints `local_ip`, which is empty until DHCP succeeds.

## Web control and telemetry

After Wi-Fi gets an address, open `http://<local_ip>/`. Port 80 exposes:

| Endpoint | Purpose |
|---|---|
| `/` | Touch control page |
| `/cmd?move=forward|backward|left|right|center|stop` | Manual command API |
| `/cmd?move=center_offset_inc|center_offset_dec` | Adjust and persist servo center by 1 degree |
| `/servo/offset` | Current center offset |
| `/telemetry` | CSV telemetry export |
| `/telemetry/reset` | Clear telemetry buffer |

Example non-motion checks:

```bash
curl "http://<local_ip>/servo/offset"
curl "http://<local_ip>/telemetry" -o carbot-telemetry.csv
```

Motion endpoints execute immediately. Do not invoke them during automated smoke tests or with the car resting on its wheels.

## micro-ROS host workflow

Firmware defaults: UDP transport, Agent `<agent_ip>:8888`, ROS domain 0, node `/carbot_base`, subscription `/cmd_vel`, `geometry_msgs/msg/Twist`, 500 ms command watchdog.

The host is expected to provide ROS 2 Humble and `micro_ros_agent`. Installation is outside this repository. On a configured Jetson:

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888 -v6
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

The firmware retry/cleanup loop should retry support initialization while Wi-Fi is connected and tear down on executor failure, but late Agent startup and restart recovery remain hardware acceptance items until the scenarios in `TODO_MICRO_ROS_RECONNECT.md` pass.

## Troubleshooting ladder

1. Build: confirm `IDF_PATH`, ESP-IDF 5.4.4, and the first compiler error. Avoid diagnosing cascaded errors first.
2. Flash: confirm the actual `/dev/cu.*` port, close other monitors, reconnect USB, and retry.
3. Boot: capture from `=== CARBOT START ===`; check reset reason, battery voltage/alarm, and the last milestone printed.
4. CLI: confirm UART0/115200 and press Enter. Startup logs and CLI share the port.
5. Wi-Fi: use `show`; verify 2.4 GHz, WPA2-compatible credentials, DHCP, and nonempty `local_ip`. Firmware retries only ten disconnect events before setting failure.
6. Web: ping/open the IP from a device on the same network, then query `/servo/offset` before any motion endpoint.
7. Agent: verify UDP port, host firewall/routing, Agent bind output, `ROS_DOMAIN_ID=0`, and that Agent starts before the device.
8. ROS: inspect node/topic/type and Agent output. A single command expires after 500 ms by design.
9. Motion: compare telemetry target, actual RPM, and PWM. Target with zero actual suggests encoder/mechanical trouble; large PWM with low actual suggests load, dead zone, battery, wiring, or stall.
10. Logs: most `ESP_LOGI/W` messages are suppressed by `esp_log_level_set("*", ESP_LOG_ERROR)`. Temporarily raise the global or per-tag level for diagnosis, then decide whether the change should remain.

## Verification matrix

Select relevant rows for each change and record actual evidence:

| Scenario | Expected result |
|---|---|
| Build | `idf.py build` succeeds and partition size remains valid |
| Healthy boot | Short beep, no alarm loop, initialization milestones complete |
| Low battery | Motor blocked/braked, servo centered, buzzer on, LED blinking |
| Wi-Fi absent | CLI remains available; no motion starts |
| Web press/release | Direction acts only while intended; release stops/centers |
| Servo offset | Changes by 1 degree, persists after reboot, remains within ±30 |
| ROS single message | Motion begins and stops/centers after about 500 ms |
| Web after ROS | Web takes ownership; later ROS timeout does not stop Web motion |
| Agent/Wi-Fi loss | ROS-owned motion stops and centers |
| Telemetry | Signs and magnitudes match commanded direction and hardware movement |

Do not claim Agent recovery, battery cutoff accuracy, track calibration, or PID stability without real hardware observations.
