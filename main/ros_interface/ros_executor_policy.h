#pragma once

#include <stdbool.h>

static inline bool ros_executor_spin_result_is_normal(bool completed,
                                                      bool timed_out)
{
    return completed || timed_out;
}
