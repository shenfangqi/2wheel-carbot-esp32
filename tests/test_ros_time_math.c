#include <assert.h>
#include <stdint.h>

#include "ros_interface/ros_time_math.h"

int main(void)
{
    assert(ros_time_apply_offset(1234567, INT64_C(1000000000)) == INT64_C(2234567000));
    assert(ros_time_apply_offset(10, -20000) == 0);
    assert(ros_time_slew_offset(1000, 5000, 20000, 1000) == 5000);
    assert(ros_time_slew_offset(1000, 50000, 20000, 1000) == 21000);
    assert(ros_time_slew_offset(50000, 1000, 20000, 1000) == 30000);
    assert(ros_time_slew_offset(-50000, -1000, 20000, 1000) == -30000);
    assert(ros_time_slew_offset(-1000, -50000, 20000, 1000) == -21000);
    assert(ros_time_make_strictly_monotonic(100, UINT64_MAX) == 100);
    assert(ros_time_make_strictly_monotonic(101, 100) == 101);
    assert(ros_time_make_strictly_monotonic(100, 100) == 101);
    assert(ros_time_make_strictly_monotonic(99, 100) == 101);
    return 0;
}
