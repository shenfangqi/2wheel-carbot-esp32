#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SERVO_GPIO_S1                    8

#define SERVO_TIMER_GROUP_ID             1
#define SERVO_TIMEBASE_RESOLUTION_HZ     1000000
#define SERVO_TIMEBASE_PERIOD_TICKS      20000
#define SERVO_PWM_FREQUENCY_HZ           50

#define SERVO_MIN_PULSEWIDTH_US          500
#define SERVO_MAX_PULSEWIDTH_US          2500
#define SERVO_MIN_ANGLE_DEG              (-90)
#define SERVO_MAX_ANGLE_DEG              (90)
#define SERVO_DEFAULT_ANGLE_DEG          0

void servo_driver_init(void);
void servo_driver_set_angle(int angle_deg);
int servo_driver_get_angle(void);

#ifdef __cplusplus
}
#endif
