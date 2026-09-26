#pragma once

#include <stdbool.h>

typedef enum {
    BATTERY_SAFETY_NORMAL = 0,
    BATTERY_SAFETY_ALARM,
} battery_safety_state_t;

typedef enum {
    BATTERY_SAFETY_NO_CHANGE = 0,
    BATTERY_SAFETY_ENTER_ALARM,
    BATTERY_SAFETY_EXIT_ALARM,
} battery_safety_transition_t;

static inline battery_safety_transition_t battery_safety_update(
    battery_safety_state_t *state, bool battery_low)
{
    if (battery_low && *state == BATTERY_SAFETY_NORMAL) {
        *state = BATTERY_SAFETY_ALARM;
        return BATTERY_SAFETY_ENTER_ALARM;
    }
    if (!battery_low && *state == BATTERY_SAFETY_ALARM) {
        *state = BATTERY_SAFETY_NORMAL;
        return BATTERY_SAFETY_EXIT_ALARM;
    }
    return BATTERY_SAFETY_NO_CHANGE;
}
