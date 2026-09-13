#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// 官方 GPIO 定义
#define PWM_GPIO_M1A              4
#define PWM_GPIO_M1B              5

#define PWM_GPIO_M2A              15
#define PWM_GPIO_M2B              16

#define PWM_GPIO_M3A              9
#define PWM_GPIO_M3B              10

#define PWM_GPIO_M4A              13
#define PWM_GPIO_M4B              14

#define PWM_MOTOR_TIMER_RESOLUTION_HZ    10000000
#define PWM_MOTOR_FREQ_HZ                25000
#define PWM_MOTOR_DUTY_TICK_MAX          (PWM_MOTOR_TIMER_RESOLUTION_HZ / PWM_MOTOR_FREQ_HZ)
#define PWM_MOTOR_DEAD_ZONE              (240)
#define PWM_MOTOR_INPUT_MAX_VALUE        (PWM_MOTOR_DUTY_TICK_MAX - PWM_MOTOR_DEAD_ZONE)

#define PWM_MOTOR_TIMER_GROUP_ID_M1      (0)
#define PWM_MOTOR_TIMER_GROUP_ID_M2      (0)
#define PWM_MOTOR_TIMER_GROUP_ID_M3      (0)
#define PWM_MOTOR_TIMER_GROUP_ID_M4      (1)

typedef enum _motor_id {
    MOTOR_ID_ALL = 0,
    MOTOR_ID_M1 = 1,
    MOTOR_ID_M2 = 2,
    MOTOR_ID_M3 = 3,
    MOTOR_ID_M4 = 4
} motor_id_t;

typedef enum _stop_mode {
    STOP_COAST = 0,
    STOP_BRAKE = 1
} stop_mode_t;

void motor_driver_init(void);
void motor_driver_set_speed_all(int speed_1, int speed_2, int speed_3, int speed_4);
void motor_driver_set_speed(motor_id_t motor_id, int speed);
void motor_driver_stop(motor_id_t motor_id, bool brake);

#ifdef __cplusplus
}
#endif
