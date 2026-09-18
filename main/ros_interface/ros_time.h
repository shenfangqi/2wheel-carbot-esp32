#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <builtin_interfaces/msg/time.h>

void ros_time_reset(void);
bool ros_time_sync(uint32_t timeout_ms);
bool ros_time_is_synchronized(void);
int64_t ros_time_get_offset_ns(void);
uint64_t ros_time_get_last_sync_age_ms(void);
uint32_t ros_time_get_sync_fail_count(void);
void ros_time_stamp_from_device_us(uint64_t device_us, uint64_t *previous_stamp_ns,
                                   builtin_interfaces__msg__Time *stamp);
