#include "control/ackermann_controller.h"

#include <math.h>

#include "control/motor_pid_controller.h"
#include "control/servo_controller.h"

// Single-servo steering uses a bicycle-model Ackermann approximation here.
// Track width is useful later for tighter odom and tire-path modeling, but
// the steering-angle mapping itself is driven primarily by wheelbase.
static const float ACKERMANN_WHEELBASE_M = 0.185f;
static const float ACKERMANN_WHEEL_RADIUS_M = 0.050f;
static const float ACKERMANN_MAX_LINEAR_MPS = 1.0f;
static const float ACKERMANN_MIN_LINEAR_FOR_STEER_MPS = 0.05f;
static const float PI_F = 3.14159265358979323846f;

static float ackermann_clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static int ackermann_round_to_int(float value)
{
    if (value >= 0.0f) {
        return (int)(value + 0.5f);
    }

    return (int)(value - 0.5f);
}

static float ackermann_linear_to_wheel_rpm(float linear_mps)
{
    float max_wheel_rpm = 0.0f;
    float wheel_rpm = 0.0f;

    if (ACKERMANN_WHEEL_RADIUS_M <= 0.0f) {
        return 0.0f;
    }

    linear_mps = ackermann_clamp_float(linear_mps, -ACKERMANN_MAX_LINEAR_MPS, ACKERMANN_MAX_LINEAR_MPS);
    max_wheel_rpm = (ACKERMANN_MAX_LINEAR_MPS * 60.0f) / (2.0f * PI_F * ACKERMANN_WHEEL_RADIUS_M);
    wheel_rpm = (linear_mps * 60.0f) / (2.0f * PI_F * ACKERMANN_WHEEL_RADIUS_M);
    return ackermann_clamp_float(wheel_rpm, -max_wheel_rpm, max_wheel_rpm);
}

static int ackermann_angular_to_steering_angle_deg(float linear_mps, float angular_rps)
{
    float steering_angle_deg = 0.0f;

    if (angular_rps == 0.0f) {
        return 0;
    }

    if (fabsf(linear_mps) < ACKERMANN_MIN_LINEAR_FOR_STEER_MPS) {
        return (angular_rps > 0.0f) ? SERVO_STEER_MAX_ANGLE_DEG : SERVO_STEER_MIN_ANGLE_DEG;
    }

    steering_angle_deg = atanf((ACKERMANN_WHEELBASE_M * angular_rps) / linear_mps) * (180.0f / PI_F);
    steering_angle_deg = ackermann_clamp_float(
        steering_angle_deg,
        (float)SERVO_STEER_MIN_ANGLE_DEG,
        (float)SERVO_STEER_MAX_ANGLE_DEG);

    return ackermann_round_to_int(steering_angle_deg);
}

void ackermann_controller_init(void)
{
    motor_pid_controller_init();
    servo_controller_center();
}

void ackermann_controller_set_cmd(float linear_mps, float angular_rps)
{
    float wheel_rpm = ackermann_linear_to_wheel_rpm(linear_mps);
    int steering_angle_deg = ackermann_angular_to_steering_angle_deg(linear_mps, angular_rps);

    servo_controller_set_angle(steering_angle_deg);
    motor_pid_controller_set_target_rpm(-wheel_rpm, wheel_rpm);
}

void ackermann_controller_stop(void)
{
    motor_pid_controller_stop(true);
}
