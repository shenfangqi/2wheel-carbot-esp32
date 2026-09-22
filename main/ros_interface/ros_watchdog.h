#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline bool ros_watchdog_is_expired(bool subscriber_initialized,
                                           int64_t last_command_ms,
                                           int64_t now_ms,
                                           int32_t timeout_ms)
{
    return subscriber_initialized &&
           timeout_ms > 0 &&
           last_command_ms >= 0 &&
           now_ms >= last_command_ms &&
           (now_ms - last_command_ms) >= timeout_ms;
}
