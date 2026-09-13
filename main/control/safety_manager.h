#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CARBOT_MAX_LINEAR_MPS 0.50f
#define CARBOT_MAX_ANGULAR_RPS 3.50f
#define CARBOT_MAX_LINEAR_ACCEL_MPS2 0.50f
#define CARBOT_MAX_ANGULAR_ACCEL_RPS2 2.50f

typedef struct {
    float linear_mps;
    float angular_rps;
    int64_t stamp_us;
    bool initialized;
} safety_limiter_t;

bool safety_manager_validate(float linear_mps, float angular_rps);
void safety_manager_reset_limiter(safety_limiter_t *limiter);
void safety_manager_limit_acceleration(safety_limiter_t *limiter,
                                       float requested_linear_mps,
                                       float requested_angular_rps,
                                       int64_t now_us,
                                       float *limited_linear_mps,
                                       float *limited_angular_rps);

#ifdef __cplusplus
}
#endif
