#include "control/command_mux.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "control/ackermann_controller.h"
#include "control/servo_controller.h"

static portMUX_TYPE s_command_mux_lock = portMUX_INITIALIZER_UNLOCKED;
static command_source_t s_active_source = COMMAND_SOURCE_NONE;

static void command_mux_set_source(command_source_t source)
{
    portENTER_CRITICAL(&s_command_mux_lock);
    s_active_source = source;
    portEXIT_CRITICAL(&s_command_mux_lock);
}

void command_mux_init(void)
{
    command_mux_set_source(COMMAND_SOURCE_NONE);
}

void command_mux_apply_web_cmd(float linear_mps, float angular_rps)
{
    command_mux_set_source(COMMAND_SOURCE_WEB);
    ackermann_controller_set_cmd(linear_mps, angular_rps);
}

void command_mux_stop_web(bool center_steering)
{
    command_mux_set_source(COMMAND_SOURCE_WEB);
    ackermann_controller_stop();
    if (center_steering) {
        servo_controller_center();
    }
}

void command_mux_apply_ros_cmd(float linear_mps, float angular_rps)
{
    command_mux_set_source(COMMAND_SOURCE_ROS);
    ackermann_controller_set_cmd(linear_mps, angular_rps);
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

    ackermann_controller_stop();
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
