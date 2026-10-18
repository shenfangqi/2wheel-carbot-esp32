#pragma once

#include <stdbool.h>

typedef enum {
    BATTERY_SAFETY_NORMAL = 0,
    BATTERY_SAFETY_ABSENT,
    BATTERY_SAFETY_LOW,
} battery_safety_state_t;

static inline battery_safety_state_t battery_safety_classify(
    bool battery_present, bool battery_low)
{
    if (!battery_present) {
        return BATTERY_SAFETY_ABSENT;
    }
    if (battery_low) {
        return BATTERY_SAFETY_LOW;
    }
    return BATTERY_SAFETY_NORMAL;
}

static inline bool battery_safety_blocks_motion(battery_safety_state_t state)
{
    return state != BATTERY_SAFETY_NORMAL;
}

static inline bool battery_safety_should_alarm(battery_safety_state_t state)
{
    return state == BATTERY_SAFETY_LOW;
}
