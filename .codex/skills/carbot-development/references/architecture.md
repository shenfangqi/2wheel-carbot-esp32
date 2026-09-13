# Architecture reference

## Contents

- Boot sequence
- Module map
- Command and data flow
- Configuration and concurrency
- Safety semantics

## Boot sequence

`app_main()` performs these operations in order:

1. Initialize NVS and restrict global ESP logging to `ERROR`.
2. Initialize status LED, buzzer, and the battery ADC task.
3. Wait up to 1 second for a battery sample. A healthy battery produces a 120 ms beep. Voltage at or below 6.60 V enters a permanent alarm loop before the rest of the system starts.
4. Load defaults and then NVS overrides.
5. Start the UART0 CLI at 115200 baud.
6. Initialize the legacy servo and apply its persisted center offset; it is not part of current chassis kinematics.
7. Initialize IMU, differential control, command arbitration, encoders, four motor driver handles, and the M1/M3 PID task.
8. Set shared PID gains to the values in `app_main.c`.
9. Initialize the telemetry ring buffer.
10. Initialize Wi-Fi STA and wait up to 15 seconds for connection/failure. The HTTP server starts on receipt of an IP address.
11. Start the micro-ROS task when enabled by Kconfig.
12. In the main loop, check runtime low voltage and collect PID/IMU telemetry every 100 ms.

Because Wi-Fi initialization waits, the CLI is deliberately started before it and remains usable when networking fails.

## Module map

| Area | Main responsibility |
|---|---|
| `main/app_config` | Runtime config defaults, NVS namespace `carbot`, UART CLI |
| `main/drivers` | MCPWM motor output, PCNT encoders, servo PWM, ADC battery, LED, buzzer |
| `main/control` | Differential/skid-steer conversion, M1/M3 speed PID, IMU heading correction, command ownership; legacy servo compatibility |
| `main/network` | Wi-Fi STA, HTTP server, UDP micro-ROS address |
| `main/ros_interface` | micro-ROS task, node `carbot_base`, `cmd_vel` subscriber and watchdog |
| `main/utils` | Telemetry ring buffer and small utility headers |
| `components/icm42670p`, `components/inv_imu`, `components/i2c_master` | Vendored/local IMU stack |
| `components/micro_ros_espidf_component` | Vendored micro-ROS ESP-IDF component |

`odometry_estimator.*` and `safety_manager.*` exist but are not registered in `main/CMakeLists.txt` and are not active. `ros_publishers.c` currently creates no publisher.

## Command and data flow

```text
Web /cmd --------------------+
                             v
ROS cmd_vel -> subscriber -> command_mux -> differential_controller
                                            |              |
                                            v              v
                                  motor_pid_controller   IMU heading correction
                                      |        |
                                      v        v
                                  encoder   motor PWM
```

- Web commands use `0.40 m/s` and `2.0 rad/s` fixed magnitudes; turning is produced by left/right track speed difference.
- ROS subscribes to relative topic `cmd_vel`. With the default empty namespace, the effective topic is `/cmd_vel`.
- `command_mux` tracks only the latest source. A Web command can take ownership after ROS and vice versa.
- `command_mux_stop_ros()` acts only if ROS still owns motion. This prevents a delayed ROS timeout from stopping a later Web command.
- `command_mux_stop_web()` sets Web as active and stops immediately.

## Configuration and concurrency

Runtime config fields are `wifi_ssid`, `wifi_password`, `agent_ip`, `agent_port`, and `servo_center_offset_deg`. Defaults are defined in `app_config.c`; NVS overrides use namespace `carbot` and keys `ssid`, `pwd`, `ip`, `port`, and `servo_ofs`.

Active asynchronous work includes:

- UART CLI task, priority 5.
- Battery task pinned to core 1, priority 2, 100 ms period.
- Motor PID task pinned to core 1, priority 10, 10 ms period.
- micro-ROS task, default priority 5 and 16 KiB stack.
- ESP-IDF Wi-Fi/event and HTTP server tasks.
- Main loop telemetry sampling every 100 ms.

Critical sections protect command ownership/motion block state and PID shared state. Review lock scope carefully when adding calls; do not invoke complex driver operations while holding a critical section.

## Safety semantics

- Startup low voltage: buzzer stays on, LED toggles every 200 ms, and startup does not proceed.
- Runtime low voltage: motion is globally blocked, both tracks brake, buzzer stays on, LED toggles, and recovery requires reset/power cycle even if voltage rises. Legacy servo centering is incidental.
- ROS watchdog: after the first received command, no new message for 500 ms stops ROS-owned track motion.
- Wi-Fi or ROS teardown calls the ROS stop path.
- Web pointer release sends stop, but network loss can prevent the release request from arriving; treat the red stop button and physical power removal as safety layers, not proof of a hard real-time remote stop.
