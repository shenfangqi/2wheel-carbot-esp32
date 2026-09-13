#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CARBOT_FAST_PUBLISH_PERIOD_MS 20
#define CARBOT_SLOW_PUBLISH_DIVIDER 25

static inline bool carbot_is_slow_publish_cycle(uint32_t cycle)
{
    return cycle != 0 && (cycle % CARBOT_SLOW_PUBLISH_DIVIDER) == 0;
}
