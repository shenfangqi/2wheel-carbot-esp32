#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_STEER_MIN_ANGLE_DEG        (-35)
#define SERVO_STEER_MAX_ANGLE_DEG        (35)
#define SERVO_STEER_LEFT_ANGLE_DEG       (25)
#define SERVO_STEER_RIGHT_ANGLE_DEG      (-25)
#define SERVO_STEER_DEFAULT_CENTER_OFFSET_DEG (10)
#define SERVO_STEER_MIN_CENTER_OFFSET_DEG (-30)
#define SERVO_STEER_MAX_CENTER_OFFSET_DEG (30)
#define SERVO_STEER_DIRECTION_SIGN       (-1)

void servo_controller_init(void);
void servo_controller_set_angle(int steering_angle_deg);
void servo_controller_center(void);
void servo_controller_turn_left(void);
void servo_controller_turn_right(void);
int servo_controller_get_angle(void);
void servo_controller_set_center_offset(int center_offset_deg);
int servo_controller_adjust_center_offset(int delta_deg);
int servo_controller_get_center_offset(void);

#ifdef __cplusplus
}
#endif
