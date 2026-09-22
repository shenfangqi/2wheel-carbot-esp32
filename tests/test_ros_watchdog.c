#include <assert.h>
#include <stdint.h>

#include "ros_interface/ros_watchdog.h"

int main(void)
{
    assert(!ros_watchdog_is_expired(false, 0, 1000, 500));
    assert(!ros_watchdog_is_expired(true, -1, 1000, 500));
    assert(!ros_watchdog_is_expired(true, 1000, 1499, 500));
    assert(ros_watchdog_is_expired(true, 1000, 1500, 500));
    assert(ros_watchdog_is_expired(true, 1000, 1800, 500));
    assert(!ros_watchdog_is_expired(true, 1000, 999, 500));
    assert(!ros_watchdog_is_expired(true, 1000, 1500, 0));
    return 0;
}
