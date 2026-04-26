#include "control/servo_controller.h"

#include "drivers/servo_driver.h"

static int servo_controller_current_angle_deg = 0;

static int servo_controller_limit_angle(int steering_angle_deg)
{
    if (steering_angle_deg > SERVO_STEER_MAX_ANGLE_DEG) return SERVO_STEER_MAX_ANGLE_DEG;
    if (steering_angle_deg < SERVO_STEER_MIN_ANGLE_DEG) return SERVO_STEER_MIN_ANGLE_DEG;
    return steering_angle_deg;
}

void servo_controller_init(void)
{
    servo_driver_init();
    servo_controller_center();
}

void servo_controller_set_angle(int steering_angle_deg)
{
    int hardware_angle_deg = 0;

    steering_angle_deg = servo_controller_limit_angle(steering_angle_deg);
    hardware_angle_deg = steering_angle_deg * SERVO_STEER_DIRECTION_SIGN + SERVO_STEER_CENTER_OFFSET_DEG;

    servo_driver_set_angle(hardware_angle_deg);
    servo_controller_current_angle_deg = steering_angle_deg;
}

void servo_controller_center(void)
{
    servo_controller_set_angle(0);
}

void servo_controller_turn_left(void)
{
    servo_controller_set_angle(SERVO_STEER_LEFT_ANGLE_DEG);
}

void servo_controller_turn_right(void)
{
    servo_controller_set_angle(SERVO_STEER_RIGHT_ANGLE_DEG);
}

int servo_controller_get_angle(void)
{
    return servo_controller_current_angle_deg;
}
