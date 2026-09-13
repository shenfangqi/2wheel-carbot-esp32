---
name: carbot-development
description: Understand, build, flash, debug, test, and extend the tracked differential-drive Carbot ESP32-S3 firmware in this repository. Use for Carbot architecture questions, ESP-IDF build failures, serial/Wi-Fi/Web/micro-ROS bring-up, motor/encoder/PID/IMU/battery work, control or safety changes, hardware pin changes, and roadmap continuation.
---

# Carbot Development

Work from the repository root. Treat source code and `sdkconfig` as the current truth; treat `TODO_*.md` as plans or historical observations that may be stale.

## Start every task

1. Read `git status --short --branch` and preserve unrelated user changes.
2. Read the files directly involved in the request plus the relevant reference below.
3. Before any motion test, require the tracks to be raised, verify a working stop path, and keep clear of the drivetrain.
4. Keep hardware constants centralized in the existing driver/controller headers; do not duplicate pin or calibration values in new modules.
5. After changing C/CMake/Kconfig code, load ESP-IDF 5.4.4 and run `idf.py build`. Run `git diff --check` for documentation-only changes.
6. Report what was verified in software separately from what still requires hardware testing.

## Load references selectively

- Read [architecture.md](references/architecture.md) for boot order, module ownership, command flow, concurrency, safety behavior, and configuration persistence.
- Read [hardware-and-control.md](references/hardware-and-control.md) before changing GPIO, PWM, encoder signs, effective track width, sprocket radius, battery thresholds, differential conversion, or PID behavior.
- Read [build-run-debug.md](references/build-run-debug.md) for exact build, flash, serial CLI, Web, telemetry, ROS 2, and troubleshooting procedures.
- Read [roadmap.md](references/roadmap.md) before selecting or implementing follow-up work; reconcile it with the current source and TODO documents first.

## Change workflow

### Diagnose

Reproduce with the narrowest available layer: build output, serial CLI, HTTP endpoint, telemetry CSV, then ROS graph/topic. Identify whether the fault belongs to hardware, driver, controller, command source, transport, or host. Do not modify code unless asked.

### Implement

Follow the existing boundaries:

- `drivers/`: GPIO/peripheral access only.
- `control/`: vehicle semantics, closed-loop control, arbitration, and safety.
- `network/`: Wi-Fi, HTTP, and micro-ROS transport configuration.
- `ros_interface/`: ROS entities, topics, callbacks, lifecycle, and timeout handling.
- `app_config/`: defaults, NVS persistence, and UART CLI.
- `app_main.c`: initialization and top-level supervision; avoid putting feature logic here when a module owns it.

Preserve these invariants:

- Low battery blocks motion, brakes both tracks, sounds the buzzer, and blinks the status LED. Centering the legacy servo is not part of the tracked-drive kinematics.
- A stale ROS `cmd_vel` stops only ROS-owned motion; it must not unexpectedly cancel current Web ownership.
- Motor M1 and M3 signs are intentionally asymmetric in the differential layer: M1 is the right track and M3 is the left track.
- The M3 encoder sign correction is intentional.
- Legacy servo center offset remains persisted in NVS for compatibility but is not a steering input for the current tracked chassis.

### Verify

Use the smallest relevant set, then expand with risk:

```bash
source /Users/shenfangqi/esp/esp-idf-v5.4.4/export.sh
idf.py build
git diff --check
```

For hardware-facing changes, provide a test matrix covering startup, stop behavior, forward/reverse, Web/ROS ownership, communication loss, and low voltage as applicable. Never claim these passed without actual device evidence.

## Known limitations

- The repository documents ROS 2 Humble on Jetson and UDP agent port 8888, but does not install or own the host environment.
- Agent late-start and reconnect logic exists in firmware, but `TODO_MICRO_ROS_RECONNECT.md` says the complete scenarios still need real-device validation.
- Publishers and odometry are placeholders/not wired into the active application.
- PID parameters are hard-coded to `0.8/0.05/0.0`; runtime tuning and per-wheel gains are not implemented.
- `app_main.c` sets global ESP logging to `ESP_LOG_ERROR`; temporarily raising log levels may be required for diagnosis.
