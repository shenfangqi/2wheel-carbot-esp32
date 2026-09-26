#include <assert.h>

#include "control/battery_safety_policy.h"

int main(void)
{
    battery_safety_state_t state = BATTERY_SAFETY_NORMAL;

    assert(battery_safety_update(&state, false) == BATTERY_SAFETY_NO_CHANGE);
    assert(state == BATTERY_SAFETY_NORMAL);
    assert(battery_safety_update(&state, true) == BATTERY_SAFETY_ENTER_ALARM);
    assert(state == BATTERY_SAFETY_ALARM);
    assert(battery_safety_update(&state, true) == BATTERY_SAFETY_NO_CHANGE);
    assert(state == BATTERY_SAFETY_ALARM);
    assert(battery_safety_update(&state, false) == BATTERY_SAFETY_EXIT_ALARM);
    assert(state == BATTERY_SAFETY_NORMAL);
    return 0;
}
