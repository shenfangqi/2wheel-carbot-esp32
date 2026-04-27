#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    COMMAND_SOURCE_NONE = 0,
    COMMAND_SOURCE_WEB,
    COMMAND_SOURCE_ROS,
} command_source_t;

void command_mux_init(void);
void command_mux_apply_web_cmd(float linear_mps, float angular_rps);
void command_mux_stop_web(bool center_steering);
void command_mux_apply_ros_cmd(float linear_mps, float angular_rps);
void command_mux_stop_ros(bool center_steering);
command_source_t command_mux_get_active_source(void);

#ifdef __cplusplus
}
#endif
