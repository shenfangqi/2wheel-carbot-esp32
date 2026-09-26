#include <assert.h>

#include "ros_interface/ros_executor_policy.h"

int main(void)
{
    assert(ros_executor_spin_result_is_normal(true, false));
    assert(ros_executor_spin_result_is_normal(false, true));
    assert(!ros_executor_spin_result_is_normal(false, false));
    return 0;
}
