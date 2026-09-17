#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline bool ros_health_check_due(int64_t now_ms, int64_t last_check_ms,
                                        uint32_t interval_ms)
{
    return now_ms - last_check_ms >= (int64_t)interval_ms;
}

static inline bool ros_health_record_ping_result(uint32_t *consecutive_failures,
                                                 bool ping_succeeded,
                                                 uint32_t failure_limit)
{
    if (ping_succeeded) {
        *consecutive_failures = 0;
        return false;
    }

    if (*consecutive_failures < UINT32_MAX) {
        (*consecutive_failures)++;
    }
    return *consecutive_failures >= failure_limit;
}
