# Hardware and control reference

## Contents

- Board and pins
- Motor and encoder behavior
- Differential/skid-steer model and legacy servo
- Battery monitoring
- PID and telemetry

## Board and pins

Target: ESP32-S3, 4 MB flash, DIO at 80 MHz. UART0 is the CP2102-backed micro-ROS link at 921600 baud; production firmware has no text monitor on that port.

| Function | GPIO/configuration |
|---|---|
| Motor M1 A/B | 4 / 5 |
| Motor M2 A/B | 15 / 16 |
| Motor M3 A/B | 9 / 10 |
| Motor M4 A/B | 13 / 14 |
| Encoder H1 A/B | 6 / 7 |
| Encoder H2 A/B | 47 / 48 |
| Encoder H3 A/B | 11 / 12 |
| Encoder H4 A/B | 1 / 2 |
| Legacy steering servo S1 (not a kinematic steering source) | 8 |
| Battery ADC | GPIO 3, ADC1 channel 2 |
| Status LED | 45, active high |
| Buzzer | 46, active high |

Confirm the physical board schematic before changing pin assignments. Some ESP32-S3 pins have boot/USB/flash implications that the source alone cannot establish for a custom board.

## Motor and encoder behavior

- MCPWM resolution: 10 MHz; PWM frequency: 25 kHz; period: 400 ticks.
- Driver command range: `-200..200`. A nonzero command receives a fixed 200-tick dead-zone offset, producing hardware duty magnitudes `201..400`.
- M1 (right track) and M3 (left track) are the active drives in the closed loop. All four motor peripherals are initialized, but M2/M4 are not commanded by the differential PID path.
- Encoders use quadrature PCNT with accumulated counts, limits ±1000, and a 1000 ns glitch filter.
- M3 swaps A/B during initialization and negates the returned count. Preserve this confirmed direction correction unless hardware evidence proves the wiring changed.
- The control calculation assumes 1040 encoder pulses per revolution and samples every 10 ms. At that interval one pulse corresponds to about 5.77 RPM, explaining low-speed quantization.

## Differential/skid-steer model

- M1 is the right track and M3 is the left track.
- Effective sprocket radius is 0.02175 m, calibrated from ground-distance tests.
- Effective differential track width is 0.254 m, calibrated to ground turning behavior including track scrub.
- Physical track-center spacing is 0.225 m. Use it for visual/collision geometry, not calibrated control or odometry equations.
- Track speeds are `left = v - w*0.254/2` and `right = v + w*0.254/2`.
- Linear speed to RPM is `rpm = mps * 60 / (2*pi*0.02175)`.
- M1 target is positive and M3 target is negative for forward vehicle motion because of the installed motor orientations.
- The legacy servo remains initialized and its offset persists for compatibility, but it is not a kinematic steering source.
- URDF/Nav2/Isaac integrations derive base motion from differential or skid-steer kinematics. Track visual joints remain separate from odometry.

## Battery monitoring

- Sample period: 100 ms.
- ADC attenuation: 12 dB; curve-fitting calibration is used when available, otherwise a 3.3 V/12-bit approximation is used.
- Battery scale: `GPIO voltage * 8.16`.
- Enter-low threshold: 6.60 V; release threshold in the monitor state: 6.90 V.
- Application alarm handling is latched in an infinite loop, so the release threshold does not automatically resume the application.

Treat the scale and thresholds as hardware-specific calibration. Verify them against a multimeter and the actual battery chemistry before changing safety behavior.

## PID and telemetry

- Active gains: `kp=0.8`, `ki=0.05`, `kd=0.0` set by `app_main()`.
- Shared gains apply to M1 and M3.
- Integral clamp: ±1000.
- Output is restricted from reversing relative to the target; an opposite-sign PID output becomes zero.
- Stop defaults to active braking in the command paths.
- Telemetry fields: sequence, timestamp, phase, M1 target/actual/PWM, M3 target/actual/PWM, gyro Z, IMU status.
- The ring buffer capacity is defined in `main/utils/telemetry_buffer.c`; inspect that source before making memory or retention assumptions.

Tune on raised wheels first, then low-speed ground tests. Record target RPM, actual RPM, PWM, battery voltage, surface, direction, and load. Change one parameter at a time and keep an immediate stop path.
