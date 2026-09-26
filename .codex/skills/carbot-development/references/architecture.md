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
3. Wait up to 1 second for a battery sample. A healthy battery produces a 120 ms beep. At or below 6.60 V, startup continues so communication remains available, but motion is blocked after command arbitration initializes.
4. Load defaults and then NVS overrides.
5. Initialize the legacy servo and apply its persisted center offset; it is not part of current chassis kinematics.
6. Initialize IMU, differential control, command arbitration, encoders, four motor driver handles, and the M1/M3 PID task.
7. Set shared PID gains to the values in `app_main.c`.
8. Initialize the telemetry ring buffer.
9. Start the micro-ROS task when enabled by Kconfig. Its framed custom transport owns UART0 at 921600 baud.
10. In the main loop, check runtime low voltage and collect PID/IMU telemetry every 100 ms.

## Module map

| Area | Main responsibility |
|---|---|
| `main/app_config` | Legacy servo offset default and NVS compatibility |
| `main/drivers` | MCPWM motor output, PCNT encoders, servo PWM, ADC battery, LED, buzzer |
| `main/control` | Differential/skid-steer conversion, M1/M3 speed PID, IMU heading correction, command ownership; legacy servo compatibility |
| `main/network` | UART0 custom framed micro-ROS transport |
| `main/ros_interface` | micro-ROS task, node `carbot_base`, `cmd_vel` subscriber and watchdog |
| `main/utils` | Telemetry ring buffer and small utility headers |
| `components/icm42670p`, `components/inv_imu`, `components/i2c_master` | Vendored/local IMU stack |
| `components/micro_ros_espidf_component` | Vendored micro-ROS ESP-IDF component |

`odometry_estimator.*` and `safety_manager.*` exist but are not registered in `main/CMakeLists.txt` and are not active. `ros_publishers.c` currently creates no publisher.

## Command and data flow

```text
ROS cmd_vel -> subscriber -> command_mux -> differential_controller
                                            |              |
                                            v              v
                                  motor_pid_controller   IMU heading correction
                                      |        |
                                      v        v
                                  encoder   motor PWM
```

- ROS subscribes to relative topic `cmd_vel`. With the default empty namespace, the effective topic is `/cmd_vel`.
- `command_mux` accepts only ROS motion commands. Its published source values remain wire-compatible: `NONE=0`, `ROS=2`.
- `command_mux_stop_ros()` acts only if ROS owns motion and then clears ownership.

## Configuration and concurrency

The only runtime config field is the legacy `servo_center_offset_deg`. NVS namespace `carbot` still reads/writes `servo_ofs`; historical `ssid`, `pwd`, `ip`, and `port` keys are ignored.

Active asynchronous work includes:

- Battery task pinned to core 1, priority 2, 100 ms period.
- Motor PID task pinned to core 1, priority 10, 10 ms period.
- micro-ROS task, default priority 5 and 16 KiB stack.
- Main loop telemetry sampling every 100 ms.

Critical sections protect command ownership/motion block state and PID shared state. Review lock scope carefully when adding calls; do not invoke complex driver operations while holding a critical section.

## Safety semantics

- Startup or runtime low voltage: motion is globally blocked, ownership is cleared, both tracks brake, and the buzzer/LED toggle every 200 ms. UART and micro-ROS remain active even when USB powers the controller with the vehicle battery disconnected.
- Battery recovery above 6.90 V clears the alarm and motion block, but ownership remains `NONE`; a new valid command is required.
- ROS watchdog: after the first received command, no new message for 500 ms stops ROS-owned track motion.
- Serial transport/session failure calls the ROS stop path.
- Jetson manual control must publish continuously; loss of commands triggers the 500 ms watchdog. Treat the Jetson Stop control and physical power removal as separate safety layers, not as equivalent mechanisms.
