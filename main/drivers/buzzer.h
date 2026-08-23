#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BUZZER_GPIO           46
#define BUZZER_ACTIVE_LEVEL   1

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_set(uint8_t state);
void buzzer_toggle(void);

#ifdef __cplusplus
}
#endif
