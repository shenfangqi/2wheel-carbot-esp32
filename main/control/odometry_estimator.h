#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    int64_t left_ticks;
    int64_t right_ticks;
    uint64_t sequence;
    uint64_t device_stamp_us;
    uint32_t boot_id;
} wheel_ticks_snapshot_t;

void odometry_estimator_init(void);
void odometry_estimator_sample(void);
void odometry_estimator_get_wheel_ticks(wheel_ticks_snapshot_t *out);

#ifdef __cplusplus
}
#endif
