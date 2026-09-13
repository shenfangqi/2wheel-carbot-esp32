#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_MONITOR_GPIO               3
#define BATTERY_MONITOR_VOLTAGE_SCALE      4.05f

/* Temporary 2S (7.4 V nominal, 8.4 V full) lithium battery profile. */
#define BATTERY_SERIES_CELL_COUNT           2.0f
#define BATTERY_LOW_CELL_ENTER_V            3.30f
#define BATTERY_LOW_CELL_RELEASE_V          3.45f
#define BATTERY_LOW_VOLTAGE_ENTER_V         (BATTERY_SERIES_CELL_COUNT * BATTERY_LOW_CELL_ENTER_V)
#define BATTERY_LOW_VOLTAGE_RELEASE_V       (BATTERY_SERIES_CELL_COUNT * BATTERY_LOW_CELL_RELEASE_V)

void battery_monitor_init(void);
bool battery_monitor_wait_ready(uint32_t timeout_ms);
float battery_monitor_get_voltage(void);
bool battery_monitor_is_low(void);

#ifdef __cplusplus
}
#endif
