#include <assert.h>

#include "ros_interface/publisher_schedule.h"

int main(void)
{
    assert(!carbot_is_slow_publish_cycle(0));
    assert(!carbot_is_slow_publish_cycle(24));
    assert(carbot_is_slow_publish_cycle(25));
    assert(carbot_is_slow_publish_cycle(50));
    return 0;
}
