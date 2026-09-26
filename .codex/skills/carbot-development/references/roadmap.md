# Roadmap reference

## Contents

- Current capabilities
- Open work
- Recommended sequence
- Documentation hygiene

## Current capabilities

- ESP-IDF 5.4.4 ESP32-S3 firmware builds successfully.
- UART0 is dedicated to framed micro-ROS traffic at 921600; production console and CLI are disabled.
- Motion commands enter only through micro-ROS `/cmd_vel`; the ESP32 exposes no HTTP service.
- ROS 2 `cmd_vel` reaches differential control over CP2102 USB-UART micro-ROS.
- M1/M3 encoder feedback drives a shared-gain 100 Hz speed PID.
- A 500 ms ROS watchdog stops stale commands; status values remain `NONE=0`, `ROS=2`.
- Battery voltage monitoring provides latched startup/runtime alarm behavior.
- ICM42670P gyro data is captured in telemetry when ready.

## Open work

Re-read the named TODO and current implementation before acting:

- `TODO_MICRO_ROS_RECONNECT.md`: validate device-first startup, Agent restart, USB reconnect, cleanup, and ROS command recovery. Parts of the retry state machine now exist, so update the TODO with evidence instead of assuming every item is absent.
- `TODO_MOTOR_TUNING.md`: replace raw PWM-tick semantics/dead-zone behavior with clearer, smoother control.
- `TODO_PID.md`: add runtime tuning, per-wheel gains, abnormal feedback/stall protection, controlled logging, and longer tests. Some statements refer to older `diff_drive_*` code and are stale.
- `TODO_STRAIGHT_LINE_AND_IMU.md`: mechanically calibrate first, then consider a forward-straight gyro outer loop and, if needed, independent wheel tuning.

Additional architectural gaps visible in current source:

- No active odometry pipeline or ROS publisher.
- Odometry messages are not published even though wheel tick estimation is active.
- No physical or hard real-time remote-stop guarantee; Jetson UI Stop remains a software control.
- Battery alarm does not recover without reset.
- Logging/telemetry controls are not runtime configurable.
- Host-side unit tests exist, but there is no hardware-in-the-loop harness.

## Recommended sequence

1. Establish a repeatable hardware acceptance checklist and capture baseline telemetry.
2. Validate and harden micro-ROS reconnect behavior without compromising command ownership.
3. Add safe runtime observability and PID tuning controls.
4. Improve motor dead-zone/startup behavior and tune M1/M3 independently if evidence supports it.
5. Verify track tension, drivetrain consistency, effective sprocket radius, and effective track width.
6. Add the minimal straight-line IMU outer loop only after baseline mechanics and wheel control are stable.
7. Implement odometry and ROS publishers after track calibration is trustworthy; keep track visuals separate from odometry kinematics.

## Documentation hygiene

Whenever behavior changes, update the closest user document and this skill reference in the same change. Mark hardware claims as one of: implemented, bench-tested with raised wheels, ground-tested, or unverified. Keep commands copyable and remove historical claims that no longer match source.
