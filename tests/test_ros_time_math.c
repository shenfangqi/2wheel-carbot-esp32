#include <assert.h>
#include <stdint.h>

#include "ros_interface/ros_time_math.h"

int main(void)
{
    assert(ros_time_apply_offset(1234567, INT64_C(1000000000)) == INT64_C(2234567000));
    assert(ros_time_apply_offset(10, -20000) == 0);
    return 0;
}
