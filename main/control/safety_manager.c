#include "control/safety_manager.h"

#include <math.h>
#include <stddef.h>

static float safety_clamp_delta(float requested, float previous, float maximum_delta)
{
    const float delta = requested - previous;
    if (delta > maximum_delta) {
        return previous + maximum_delta;
    }
    if (delta < -maximum_delta) {
        return previous - maximum_delta;
    }
    return requested;
}

bool safety_manager_validate(float linear_mps, float angular_rps)
{
    return isfinite(linear_mps) && isfinite(angular_rps) &&
           fabsf(linear_mps) <= CARBOT_MAX_LINEAR_MPS &&
           fabsf(angular_rps) <= CARBOT_MAX_ANGULAR_RPS;
}

void safety_manager_reset_limiter(safety_limiter_t *limiter)
{
    if (limiter != NULL) {
        *limiter = (safety_limiter_t){0};
    }
}

void safety_manager_limit_acceleration(safety_limiter_t *limiter,
                                       float requested_linear_mps,
                                       float requested_angular_rps,
                                       int64_t now_us,
                                       float *limited_linear_mps,
                                       float *limited_angular_rps)
{
    float linear = requested_linear_mps;
    float angular = requested_angular_rps;

    if (limiter != NULL && limiter->initialized && now_us > limiter->stamp_us) {
        float dt_s = (now_us - limiter->stamp_us) / 1000000.0f;
        if (dt_s > 0.5f) {
            dt_s = 0.5f;
        }
        linear = safety_clamp_delta(requested_linear_mps, limiter->linear_mps,
                                    CARBOT_MAX_LINEAR_ACCEL_MPS2 * dt_s);
        angular = safety_clamp_delta(requested_angular_rps, limiter->angular_rps,
                                     CARBOT_MAX_ANGULAR_ACCEL_RPS2 * dt_s);
    }

    if (limiter != NULL) {
        limiter->linear_mps = linear;
        limiter->angular_rps = angular;
        limiter->stamp_us = now_us;
        limiter->initialized = true;
    }
    if (limited_linear_mps != NULL) {
        *limited_linear_mps = linear;
    }
    if (limited_angular_rps != NULL) {
        *limited_angular_rps = angular;
    }
}
