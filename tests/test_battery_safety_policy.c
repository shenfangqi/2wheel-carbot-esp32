#include <assert.h>

#include "control/battery_safety_policy.h"

int main(void)
{
    battery_safety_state_t normal = battery_safety_classify(true, false);
    battery_safety_state_t absent = battery_safety_classify(false, true);
    battery_safety_state_t low = battery_safety_classify(true, true);

    assert(normal == BATTERY_SAFETY_NORMAL);
    assert(!battery_safety_blocks_motion(normal));
    assert(!battery_safety_should_alarm(normal));

    assert(absent == BATTERY_SAFETY_ABSENT);
    assert(battery_safety_blocks_motion(absent));
    assert(!battery_safety_should_alarm(absent));

    assert(low == BATTERY_SAFETY_LOW);
    assert(battery_safety_blocks_motion(low));
    assert(battery_safety_should_alarm(low));
    return 0;
}
