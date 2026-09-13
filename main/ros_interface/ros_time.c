#include "ros_interface/ros_time.h"

#include <stddef.h>

#include "esp_timer.h"
#include <rmw_microros/rmw_microros.h>

#include "ros_interface/ros_time_math.h"

static bool s_synchronized;
static int64_t s_offset_ns;

void ros_time_reset(void)
{
    s_synchronized = false;
    s_offset_ns = 0;
}

bool ros_time_sync(void)
{
    if (rmw_uros_sync_session(1000) != RMW_RET_OK) {
        ros_time_reset();
        return false;
    }

    const int64_t device_ns = esp_timer_get_time() * INT64_C(1000);
    const int64_t epoch_ns = rmw_uros_epoch_nanos();
    if (epoch_ns <= 0) {
        ros_time_reset();
        return false;
    }
    s_offset_ns = epoch_ns - device_ns;
    s_synchronized = true;
    return true;
}

bool ros_time_is_synchronized(void)
{
    return s_synchronized;
}

int64_t ros_time_get_offset_ns(void)
{
    return s_offset_ns;
}

void ros_time_stamp_from_device_us(uint64_t device_us, builtin_interfaces__msg__Time *stamp)
{
    if (stamp == NULL) {
        return;
    }
    int64_t time_ns = ros_time_apply_offset(device_us, s_synchronized ? s_offset_ns : 0);
    stamp->sec = (int32_t)(time_ns / INT64_C(1000000000));
    stamp->nanosec = (uint32_t)(time_ns % INT64_C(1000000000));
}
