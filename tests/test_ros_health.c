#include <assert.h>
#include <stdint.h>

#include "ros_interface/ros_health.h"

int main(void)
{
    uint32_t failures = 0;

    assert(!ros_health_check_due(4999, 0, 5000));
    assert(ros_health_check_due(5000, 0, 5000));
    assert(!ros_health_record_ping_result(&failures, false, 3));
    assert(failures == 1);
    assert(!ros_health_record_ping_result(&failures, true, 3));
    assert(failures == 0);
    assert(!ros_health_record_ping_result(&failures, false, 3));
    assert(!ros_health_record_ping_result(&failures, false, 3));
    assert(ros_health_record_ping_result(&failures, false, 3));
    assert(failures == 3);
    return 0;
}
