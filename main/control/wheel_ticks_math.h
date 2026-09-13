#pragma once

#include <stdint.h>

static inline int64_t wheel_ticks_accumulate(int64_t total, int previous_raw, int current_raw)
{
    const int32_t delta = (int32_t)((uint32_t)current_raw - (uint32_t)previous_raw);
    return total + (int64_t)delta;
}

static inline int64_t wheel_ticks_accumulate_signed(int64_t total, int previous_raw,
                                                    int current_raw, int direction_sign)
{
    const int32_t delta = (int32_t)((uint32_t)current_raw - (uint32_t)previous_raw);
    return total + ((int64_t)delta * direction_sign);
}
