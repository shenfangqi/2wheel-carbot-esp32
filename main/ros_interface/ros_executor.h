#pragma once

#include <stdint.h>

typedef enum {
    ROS_DISCONNECT_NONE = 0,
    ROS_DISCONNECT_TRANSPORT = 1,
    ROS_DISCONNECT_EXECUTOR = 2,
    ROS_DISCONNECT_PUBLISHER = 3,
    ROS_DISCONNECT_AGENT_PING = 4,
    ROS_DISCONNECT_ENTITY_INIT = 5,
} ros_disconnect_reason_t;

void ros_executor_start(void);
uint32_t ros_executor_get_reconnect_count(void);
ros_disconnect_reason_t ros_executor_get_last_disconnect_reason(void);
uint32_t ros_executor_get_consecutive_ping_failures(void);
uint64_t ros_executor_get_session_uptime_ms(void);
