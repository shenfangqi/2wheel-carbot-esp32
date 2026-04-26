#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_STEER_MIN_ANGLE_DEG        (-35)
#define SERVO_STEER_MAX_ANGLE_DEG        (35)
#define SERVO_STEER_LEFT_ANGLE_DEG       (25)
#define SERVO_STEER_RIGHT_ANGLE_DEG      (-25)
#define SERVO_STEER_CENTER_OFFSET_DEG    (0)
#define SERVO_STEER_DIRECTION_SIGN       (-1)

void servo_controller_init(void);
void servo_controller_set_angle(int steering_angle_deg);
void servo_controller_center(void);
void servo_controller_turn_left(void);
void servo_controller_turn_right(void);
int servo_controller_get_angle(void);

#ifdef __cplusplus
}
#endif
