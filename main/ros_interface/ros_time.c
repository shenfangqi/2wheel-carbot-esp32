#include "ros_interface/ros_time.h"

#include <stddef.h>

#include "esp_timer.h"
#include <rmw_microros/rmw_microros.h>

#include "ros_interface/ros_time_math.h"
#include "ros_interface/ros_time_state.h"

static ros_time_sync_state_t s_sync_state;
static uint64_t s_last_slew_device_us;

void ros_time_reset(void)
{
    ros_time_sync_state_reset(&s_sync_state);
    s_last_slew_device_us = 0;
}

bool ros_time_sync(uint32_t timeout_ms)
{
    if (rmw_uros_sync_session(timeout_ms) != RMW_RET_OK) {
        ros_time_sync_state_record_failure(&s_sync_state);
        return false;
    }

    const int64_t device_ns = esp_timer_get_time() * INT64_C(1000);
    const int64_t epoch_ns = rmw_uros_epoch_nanos();
    if (epoch_ns <= 0) {
        ros_time_sync_state_record_failure(&s_sync_state);
        return false;
    }
    ros_time_sync_state_record_success(
        &s_sync_state, epoch_ns - device_ns, esp_timer_get_time());
    return true;
}

bool ros_time_is_synchronized(void)
{
    return s_sync_state.synchronized;
}

int64_t ros_time_get_offset_ns(void)
{
    return s_sync_state.current_offset_ns;
}

uint64_t ros_time_get_last_sync_age_ms(void)
{
    if (s_sync_state.last_sync_us == 0) {
        return UINT64_MAX;
    }
    return (uint64_t)((esp_timer_get_time() - s_sync_state.last_sync_us) / 1000);
}

uint32_t ros_time_get_sync_fail_count(void)
{
    return s_sync_state.sync_fail_count;
}

void ros_time_stamp_from_device_us(uint64_t device_us, uint64_t *previous_stamp_ns,
                                   builtin_interfaces__msg__Time *stamp)
{
    if (stamp == NULL || previous_stamp_ns == NULL) {
        return;
    }

    if (s_last_slew_device_us != 0 && device_us > s_last_slew_device_us) {
        s_sync_state.current_offset_ns = ros_time_slew_offset(
            s_sync_state.current_offset_ns,
            s_sync_state.target_offset_ns,
            device_us - s_last_slew_device_us,
            CONFIG_CARBOT_MICRO_ROS_TIME_SLEW_PPM);
    }
    if (device_us > s_last_slew_device_us) {
        s_last_slew_device_us = device_us;
    }

    int64_t candidate_ns = ros_time_apply_offset(device_us, s_sync_state.current_offset_ns);
    uint64_t time_ns = ros_time_make_strictly_monotonic((uint64_t)candidate_ns,
                                                        *previous_stamp_ns);
    *previous_stamp_ns = time_ns;
    stamp->sec = (int32_t)(time_ns / UINT64_C(1000000000));
    stamp->nanosec = (uint32_t)(time_ns % UINT64_C(1000000000));
}
