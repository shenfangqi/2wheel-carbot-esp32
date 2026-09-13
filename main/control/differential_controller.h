#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

static inline bool differential_command_is_stopped(float linear_mps, float angular_rps)
{
    return fabsf(linear_mps) < 0.0001f && fabsf(angular_rps) < 0.0001f;
}

static inline bool differential_command_snapshot_is_current(uint32_t snapshot_generation,
                                                            uint32_t current_generation)
{
    return snapshot_generation == current_generation;
}

static inline bool differential_heading_hold_should_activate(bool enabled,
                                                             bool straight,
                                                             bool imu_calibrated,
                                                             bool imu_ok)
{
    return enabled && straight && imu_calibrated && imu_ok;
}

typedef struct {
    bool imu_calibrated;
    bool heading_active;
    float gyro_z_bias_rad_s;
    float relative_yaw_rad;
    float correction_rad_s;
} differential_heading_status_t;

void differential_controller_init(void);
void differential_controller_set_cmd(float linear_mps, float angular_rps);
void differential_controller_stop(void);
int differential_controller_distance_to_counts(float distance_m);
int differential_controller_turn_to_counts(float angle_rad);
void differential_controller_get_heading_status(differential_heading_status_t *out);

#ifdef __cplusplus
}
#endif
