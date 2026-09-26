#include "control/command_mux.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "control/differential_controller.h"
#include "control/safety_manager.h"
#include "control/servo_controller.h"

static portMUX_TYPE s_command_mux_lock = portMUX_INITIALIZER_UNLOCKED;
static command_source_t s_active_source = COMMAND_SOURCE_NONE;
static bool s_motion_blocked = false;
static uint32_t s_invalid_command_count = 0;

static void command_mux_set_source(command_source_t source)
{
    portENTER_CRITICAL(&s_command_mux_lock);
    s_active_source = source;
    portEXIT_CRITICAL(&s_command_mux_lock);
}

void command_mux_init(void)
{
    command_mux_set_source(COMMAND_SOURCE_NONE);
    portENTER_CRITICAL(&s_command_mux_lock);
    s_motion_blocked = false;
    s_invalid_command_count = 0;
    portEXIT_CRITICAL(&s_command_mux_lock);
}

void command_mux_apply_ros_cmd(float linear_mps, float angular_rps)
{
    if (!safety_manager_validate(linear_mps, angular_rps)) {
        portENTER_CRITICAL(&s_command_mux_lock);
        s_invalid_command_count++;
        const bool stop_ros = s_active_source == COMMAND_SOURCE_ROS;
        portEXIT_CRITICAL(&s_command_mux_lock);
        if (stop_ros) {
            command_mux_stop_ros(false);
        }
        return;
    }
    if (command_mux_is_motion_blocked()) {
        differential_controller_stop();
        servo_controller_center();
        return;
    }

    command_mux_set_source(COMMAND_SOURCE_ROS);
    differential_controller_set_cmd(linear_mps, angular_rps);
}

void command_mux_stop_ros(bool center_steering)
{
    portENTER_CRITICAL(&s_command_mux_lock);
    if (s_active_source != COMMAND_SOURCE_ROS) {
        portEXIT_CRITICAL(&s_command_mux_lock);
        return;
    }
    s_active_source = COMMAND_SOURCE_NONE;
    portEXIT_CRITICAL(&s_command_mux_lock);

    differential_controller_stop();
    if (center_steering) {
        servo_controller_center();
    }
}

command_source_t command_mux_get_active_source(void)
{
    command_source_t source = COMMAND_SOURCE_NONE;

    portENTER_CRITICAL(&s_command_mux_lock);
    source = s_active_source;
    portEXIT_CRITICAL(&s_command_mux_lock);

    return source;
}

void command_mux_set_motion_blocked(bool blocked)
{
    portENTER_CRITICAL(&s_command_mux_lock);
    s_motion_blocked = blocked;
    if (blocked) {
        s_active_source = COMMAND_SOURCE_NONE;
    }
    portEXIT_CRITICAL(&s_command_mux_lock);

    if (blocked) {
        differential_controller_stop();
        servo_controller_center();
    }
}

bool command_mux_is_motion_blocked(void)
{
    bool blocked = false;

    portENTER_CRITICAL(&s_command_mux_lock);
    blocked = s_motion_blocked;
    portEXIT_CRITICAL(&s_command_mux_lock);

    return blocked;
}

uint32_t command_mux_get_invalid_command_count(void)
{
    uint32_t count;
    portENTER_CRITICAL(&s_command_mux_lock);
    count = s_invalid_command_count;
    portEXIT_CRITICAL(&s_command_mux_lock);
    return count;
}
