#pragma once

#include <stdint.h>

static inline int64_t ros_time_apply_offset(uint64_t device_us, int64_t offset_ns)
{
    const int64_t device_ns = (int64_t)(device_us * UINT64_C(1000));
    const int64_t result = device_ns + offset_ns;
    return result > 0 ? result : 0;
}

static inline int64_t ros_time_slew_offset(int64_t current_ns, int64_t target_ns,
                                           uint64_t elapsed_us, uint32_t max_rate_ppm)
{
    int64_t difference_ns = target_ns - current_ns;
    uint64_t max_step_ns = elapsed_us * (uint64_t)max_rate_ppm / UINT64_C(1000);

    if (difference_ns > 0 && (uint64_t)difference_ns > max_step_ns) {
        return current_ns + (int64_t)max_step_ns;
    }
    if (difference_ns < 0 && (uint64_t)(-difference_ns) > max_step_ns) {
        return current_ns - (int64_t)max_step_ns;
    }
    return target_ns;
}

static inline uint64_t ros_time_make_strictly_monotonic(uint64_t candidate_ns,
                                                        uint64_t previous_ns)
{
    if (candidate_ns > previous_ns || previous_ns == UINT64_MAX) {
        return candidate_ns;
    }
    return previous_ns + UINT64_C(1);
}
