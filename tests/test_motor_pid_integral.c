#include <assert.h>

#include "control/motor_pid_controller.h"

int main(void)
{
    assert(motor_pid_integral_for_target(-22.0f, 500.0f) == 0.0f);
    assert(motor_pid_integral_for_target(22.0f, -500.0f) == 0.0f);
    assert(motor_pid_integral_for_target(22.0f, 500.0f) == 500.0f);
    assert(motor_pid_integral_for_target(-22.0f, -500.0f) == -500.0f);
    assert(motor_pid_integral_for_target(0.0f, 500.0f) == 0.0f);
    assert(motor_pid_integral_for_target(22.0f, 0.0f) == 0.0f);
    assert(motor_pid_integral_for_target(-22.0f, 0.0f) == 0.0f);
    return 0;
}
