#pragma once

#include <stdint.h>

static inline int64_t ros_time_apply_offset(uint64_t device_us, int64_t offset_ns)
{
    const int64_t device_ns = (int64_t)(device_us * UINT64_C(1000));
    const int64_t result = device_ns + offset_ns;
    return result > 0 ? result : 0;
}
