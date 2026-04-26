#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ENCODER_GPIO_H1A            6
#define ENCODER_GPIO_H1B            7

#define ENCODER_GPIO_H2A            47
#define ENCODER_GPIO_H2B            48

#define ENCODER_GPIO_H3A            11
#define ENCODER_GPIO_H3B            12

#define ENCODER_GPIO_H4A            1
#define ENCODER_GPIO_H4B            2

#define ENCODER_PCNT_HIGH_LIMIT     1000
#define ENCODER_PCNT_LOW_LIMIT      -1000
#define ENCODER_GLITCH_FILTER_NS    1000

typedef enum {
    ENCODER_ID_M1 = 1,
    ENCODER_ID_M2 = 2,
    ENCODER_ID_M3 = 3,
    ENCODER_ID_M4 = 4,
} encoder_id_t;

void encoder_driver_init(void);
int encoder_driver_get_count(encoder_id_t encoder_id);
int encoder_driver_get_count_m1(void);
int encoder_driver_get_count_m2(void);
int encoder_driver_get_count_m3(void);
int encoder_driver_get_count_m4(void);

#ifdef __cplusplus
}
#endif
