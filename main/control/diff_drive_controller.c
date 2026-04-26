#include "control/diff_drive_controller.h"
#include "control/motor_pid_controller.h"

void diff_drive_init(void)
{
    motor_pid_controller_init();
}

void diff_drive_backward(int speed)
{
    motor_pid_controller_set_target_rpm((float)speed, (float)-speed);
}

void diff_drive_forward(int speed)
{
    motor_pid_controller_set_target_rpm((float)-speed, (float)speed);
}

void diff_drive_stop(void)
{
    motor_pid_controller_stop(true);
}
