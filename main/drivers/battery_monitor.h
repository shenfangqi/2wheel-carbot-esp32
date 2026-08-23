#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_MONITOR_GPIO               3
#define BATTERY_MONITOR_VOLTAGE_SCALE      8.16f
#define BATTERY_LOW_VOLTAGE_ENTER_V        6.60f
#define BATTERY_LOW_VOLTAGE_RELEASE_V      6.90f

void battery_monitor_init(void);
bool battery_monitor_wait_ready(uint32_t timeout_ms);
float battery_monitor_get_voltage(void);
bool battery_monitor_is_low(void);

#ifdef __cplusplus
}
#endif
