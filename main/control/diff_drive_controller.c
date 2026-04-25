#include "control/diff_drive_controller.h"
#include "drivers/motor_driver.h"

void diff_drive_init(void)
{
    motor_driver_init();
    motor_driver_stop(MOTOR_ID_ALL, true);
}

void diff_drive_forward(int speed)
{
    motor_driver_set_speed(MOTOR_ID_M1, speed);
    motor_driver_set_speed(MOTOR_ID_M3, -speed);
}

void diff_drive_backward(int speed)
{
    motor_driver_set_speed(MOTOR_ID_M1, -speed);
    motor_driver_set_speed(MOTOR_ID_M3, speed);
}

void diff_drive_stop(void)
{
    motor_driver_stop(MOTOR_ID_M1, true);
    motor_driver_stop(MOTOR_ID_M3, true);
}
