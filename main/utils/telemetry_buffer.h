#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "control/motor_pid_controller.h"

#define TELEMETRY_PHASE_MAX_LEN 24

typedef struct {
    uint32_t sequence;
    int64_t timestamp_ms;
    char phase[TELEMETRY_PHASE_MAX_LEN];
    motor_pid_telemetry_t m1;
    motor_pid_telemetry_t m3;
    float gyro_z;
    int imu_status;
} telemetry_sample_t;

void telemetry_buffer_init(void);
void telemetry_buffer_reset(void);
void telemetry_buffer_push(const char *phase, const motor_pid_telemetry_t *m1, const motor_pid_telemetry_t *m3, float gyro_z, int imu_status);
size_t telemetry_buffer_copy_latest(telemetry_sample_t *out_samples, size_t max_samples);
size_t telemetry_buffer_count(void);
bool telemetry_buffer_initialized(void);

#ifdef __cplusplus
}
#endif
