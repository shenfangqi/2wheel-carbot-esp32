#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define STATUS_LED_GPIO           45
#define STATUS_LED_ACTIVE_LEVEL   1

void status_led_init(void);
void status_led_on(void);
void status_led_off(void);
void status_led_set(uint8_t state);
void status_led_toggle(void);

#ifdef __cplusplus
}
#endif
