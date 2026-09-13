#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define MOTOR_ENCODER_COUNTS_PER_REV 1560.0f

typedef struct {
    float target_rpm;
    float actual_rpm;
    int pwm_output;
} motor_pid_telemetry_t;

void motor_pid_controller_init(void);
void motor_pid_controller_set_pid(float kp, float ki, float kd);
void motor_pid_controller_set_target_rpm(float m1_target_rpm, float m3_target_rpm);
void motor_pid_controller_set_open_loop(int m1_pwm, int m3_pwm);
void motor_pid_controller_stop(bool brake);
void motor_pid_controller_get_m1_telemetry(motor_pid_telemetry_t *out);
void motor_pid_controller_get_m3_telemetry(motor_pid_telemetry_t *out);

#ifdef __cplusplus
}
#endif
