#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    COMMAND_SOURCE_NONE = 0,
    /* Value 1 was the removed Web source; keep the status wire protocol stable. */
    COMMAND_SOURCE_ROS = 2,
} command_source_t;

void command_mux_init(void);
void command_mux_apply_ros_cmd(float linear_mps, float angular_rps);
void command_mux_stop_ros(bool center_steering);
command_source_t command_mux_get_active_source(void);
void command_mux_set_motion_blocked(bool blocked);
bool command_mux_is_motion_blocked(void);
uint32_t command_mux_get_invalid_command_count(void);

#ifdef __cplusplus
}
#endif
