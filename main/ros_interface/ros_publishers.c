#include "ros_interface/ros_publishers.h"

#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include <carbot_msgs/msg/wheel_ticks.h>
#include <carbot_msgs/msg/carbot_status.h>
#include <rclc/publisher.h>
#include <rclc/timer.h>
#include <rosidl_runtime_c/message_type_support_struct.h>
#include <sensor_msgs/msg/battery_state.h>
#include <sensor_msgs/msg/imu.h>

#include "control/command_mux.h"
#include "control/odometry_estimator.h"
#include "drivers/battery_monitor.h"
#include "icm42670p.h"
#include "ros_interface/ros_executor.h"
#include "ros_interface/publisher_schedule.h"
#include "ros_interface/ros_time.h"
#include "ros_interface/ros_topics.h"

static const char *TAG = "ros_publishers";
static rcl_publisher_t s_wheel_ticks_publisher;
static rcl_publisher_t s_imu_publisher;
static rcl_publisher_t s_battery_publisher;
static rcl_publisher_t s_status_publisher;
static rcl_timer_t s_publish_timer;
static carbot_msgs__msg__WheelTicks s_wheel_ticks_msg;
static sensor_msgs__msg__Imu s_imu_msg;
static sensor_msgs__msg__BatteryState s_battery_msg;
static carbot_msgs__msg__CarbotStatus s_status_msg;
static bool s_wheel_ticks_ready;
static bool s_imu_ready;
static bool s_battery_ready;
static bool s_status_ready;
static bool s_timer_ready;
static bool s_healthy;
static uint32_t s_publish_cycle;
static uint64_t s_status_sequence;
static uint64_t s_wheel_ticks_last_stamp_ns;
static uint64_t s_imu_last_stamp_ns;
static uint64_t s_battery_last_stamp_ns;
static uint64_t s_status_last_stamp_ns;

static void ros_publishers_set_frame_id(std_msgs__msg__Header *header, char *frame_id)
{
    header->frame_id.data = frame_id;
    header->frame_id.size = strlen(frame_id);
    header->frame_id.capacity = header->frame_id.size + 1;
}

static void ros_publishers_timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    wheel_ticks_snapshot_t snapshot = {0};
    (void)last_call_time;

    if (timer == NULL || !s_wheel_ticks_ready) {
        return;
    }

    odometry_estimator_get_wheel_ticks(&snapshot);
    s_wheel_ticks_msg.sequence = snapshot.sequence;
    s_wheel_ticks_msg.boot_id = snapshot.boot_id;
    s_wheel_ticks_msg.device_stamp_us = snapshot.device_stamp_us;
    s_wheel_ticks_msg.left_ticks = snapshot.left_ticks;
    s_wheel_ticks_msg.right_ticks = snapshot.right_ticks;
    ros_time_stamp_from_device_us(snapshot.device_stamp_us, &s_wheel_ticks_last_stamp_ns,
                                  &s_wheel_ticks_msg.header.stamp);

    rcl_ret_t publish_result =
        rcl_publish(&s_wheel_ticks_publisher, &s_wheel_ticks_msg, NULL);
    if (publish_result != RCL_RET_OK) {
        ESP_LOGE(TAG, "wheel ticks publish failed: %d", (int)publish_result);
        s_healthy = false;
        return;
    }

    if (s_imu_ready && Icm42670p_Start_OK() > 0) {
        float acceleration_m_s2[3] = {0};
        float angular_velocity_rad_s[3] = {0};
        Icm42670p_Get_Accel_m_s2(acceleration_m_s2);
        Icm42670p_Get_Gyro_rad_s(angular_velocity_rad_s);
        ros_time_stamp_from_device_us(snapshot.device_stamp_us, &s_imu_last_stamp_ns,
                                      &s_imu_msg.header.stamp);
        s_imu_msg.angular_velocity.x = angular_velocity_rad_s[0];
        s_imu_msg.angular_velocity.y = angular_velocity_rad_s[1];
        s_imu_msg.angular_velocity.z = angular_velocity_rad_s[2];
        s_imu_msg.linear_acceleration.x = acceleration_m_s2[0];
        s_imu_msg.linear_acceleration.y = acceleration_m_s2[1];
        s_imu_msg.linear_acceleration.z = acceleration_m_s2[2];
        publish_result = rcl_publish(&s_imu_publisher, &s_imu_msg, NULL);
        if (publish_result != RCL_RET_OK) {
            ESP_LOGE(TAG, "IMU publish failed: %d", (int)publish_result);
            s_healthy = false;
            return;
        }
    }

    s_publish_cycle++;
    if (carbot_is_slow_publish_cycle(s_publish_cycle)) {
        ros_time_stamp_from_device_us(snapshot.device_stamp_us, &s_battery_last_stamp_ns,
                                      &s_battery_msg.header.stamp);
        s_battery_msg.voltage = battery_monitor_get_voltage();
        publish_result = rcl_publish(&s_battery_publisher, &s_battery_msg, NULL);
        if (publish_result != RCL_RET_OK) {
            ESP_LOGE(TAG, "battery publish failed: %d", (int)publish_result);
            s_healthy = false;
            return;
        }

        ros_time_stamp_from_device_us(snapshot.device_stamp_us, &s_status_last_stamp_ns,
                                      &s_status_msg.header.stamp);
        s_status_msg.sequence = ++s_status_sequence;
        s_status_msg.boot_id = snapshot.boot_id;
        s_status_msg.device_stamp_us = snapshot.device_stamp_us;
        s_status_msg.agent_connected = true;
        s_status_msg.time_synchronized = ros_time_is_synchronized();
        s_status_msg.motion_blocked = command_mux_is_motion_blocked();
        s_status_msg.battery_low = battery_monitor_is_low();
        s_status_msg.imu_ready = Icm42670p_Start_OK() > 0;
        s_status_msg.active_command_source = (uint8_t)command_mux_get_active_source();
        s_status_msg.invalid_cmd_count = command_mux_get_invalid_command_count();
        s_status_msg.reconnect_count = ros_executor_get_reconnect_count();
        s_status_msg.last_disconnect_reason = (uint8_t)ros_executor_get_last_disconnect_reason();
        s_status_msg.consecutive_ping_failures = ros_executor_get_consecutive_ping_failures();
        s_status_msg.session_uptime_ms = ros_executor_get_session_uptime_ms();
        s_status_msg.clock_offset_ns = ros_time_get_offset_ns();
        s_status_msg.last_time_sync_age_ms = ros_time_get_last_sync_age_ms();
        s_status_msg.time_sync_fail_count = ros_time_get_sync_fail_count();
        publish_result = rcl_publish(&s_status_publisher, &s_status_msg, NULL);
        if (publish_result != RCL_RET_OK) {
            ESP_LOGE(TAG, "status publish failed: %d", (int)publish_result);
            s_healthy = false;
        }

        if ((s_status_sequence % 10) == 0) {
            ESP_LOGI(TAG, "publish heartbeat: fast_cycle=%" PRIu32 " status_sequence=%" PRIu64,
                     s_publish_cycle, s_status_sequence);
        }
    }
}

esp_err_t ros_publishers_init(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor)
{
    static char wheel_frame[] = "base_link";
    static char imu_frame[] = "imu_link";

    if (node == NULL || support == NULL || executor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_wheel_ticks_publisher = rcl_get_zero_initialized_publisher();
    s_imu_publisher = rcl_get_zero_initialized_publisher();
    s_battery_publisher = rcl_get_zero_initialized_publisher();
    s_status_publisher = rcl_get_zero_initialized_publisher();
    s_publish_timer = rcl_get_zero_initialized_timer();
    s_wheel_ticks_msg = (carbot_msgs__msg__WheelTicks){0};
    s_imu_msg = (sensor_msgs__msg__Imu){0};
    s_battery_msg = (sensor_msgs__msg__BatteryState){0};
    s_status_msg = (carbot_msgs__msg__CarbotStatus){0};
    ros_publishers_set_frame_id(&s_wheel_ticks_msg.header, wheel_frame);
    ros_publishers_set_frame_id(&s_imu_msg.header, imu_frame);
    s_imu_msg.orientation_covariance[0] = -1.0;
    s_battery_msg.temperature = NAN;
    s_battery_msg.current = NAN;
    s_battery_msg.charge = NAN;
    s_battery_msg.capacity = NAN;
    s_battery_msg.design_capacity = NAN;
    s_battery_msg.percentage = NAN;
    s_battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_UNKNOWN;
    s_battery_msg.power_supply_health = sensor_msgs__msg__BatteryState__POWER_SUPPLY_HEALTH_UNKNOWN;
    s_battery_msg.power_supply_technology = sensor_msgs__msg__BatteryState__POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
    s_battery_msg.present = true;
    s_publish_cycle = 0;
    s_wheel_ticks_last_stamp_ns = UINT64_MAX;
    s_imu_last_stamp_ns = UINT64_MAX;
    s_battery_last_stamp_ns = UINT64_MAX;
    s_status_last_stamp_ns = UINT64_MAX;
    s_healthy = true;

    if (rclc_publisher_init_best_effort(
            &s_wheel_ticks_publisher,
            node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(carbot_msgs, msg, WheelTicks),
            ROS_TOPIC_WHEEL_TICKS) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    s_wheel_ticks_ready = true;

    if (rclc_publisher_init_best_effort(
            &s_imu_publisher, node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
            ROS_TOPIC_IMU_RAW) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    s_imu_ready = true;

    if (rclc_publisher_init_best_effort(
            &s_battery_publisher, node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
            ROS_TOPIC_BATTERY_STATE) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    s_battery_ready = true;

    if (rclc_publisher_init_best_effort(
            &s_status_publisher, node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(carbot_msgs, msg, CarbotStatus),
            ROS_TOPIC_CARBOT_STATUS) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    s_status_ready = true;

    if (rclc_timer_init_default(
            &s_publish_timer,
            support,
            RCL_MS_TO_NS(CARBOT_FAST_PUBLISH_PERIOD_MS),
            ros_publishers_timer_callback) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    s_timer_ready = true;

    if (rclc_executor_add_timer(executor, &s_publish_timer) != RCL_RET_OK) {
        s_healthy = false;
        return ESP_FAIL;
    }
    return ESP_OK;
}

void ros_publishers_fini(rcl_node_t *node, rclc_executor_t *executor)
{
    (void)executor;

    if (s_timer_ready) {
        rcl_ret_t rc = rcl_timer_fini(&s_publish_timer);
        if (rc != RCL_RET_OK) {
            ESP_LOGW(TAG, "timer cleanup failed: %d", (int)rc);
        }
        s_publish_timer = rcl_get_zero_initialized_timer();
        s_timer_ready = false;
    }
    if (s_status_ready && node != NULL) {
        rcl_ret_t rc = rcl_publisher_fini(&s_status_publisher, node);
        if (rc != RCL_RET_OK) ESP_LOGW(TAG, "status publisher cleanup failed: %d", (int)rc);
        s_status_publisher = rcl_get_zero_initialized_publisher();
        s_status_ready = false;
    }
    if (s_battery_ready && node != NULL) {
        rcl_ret_t rc = rcl_publisher_fini(&s_battery_publisher, node);
        if (rc != RCL_RET_OK) ESP_LOGW(TAG, "battery publisher cleanup failed: %d", (int)rc);
        s_battery_publisher = rcl_get_zero_initialized_publisher();
        s_battery_ready = false;
    }
    if (s_imu_ready && node != NULL) {
        rcl_ret_t rc = rcl_publisher_fini(&s_imu_publisher, node);
        if (rc != RCL_RET_OK) ESP_LOGW(TAG, "IMU publisher cleanup failed: %d", (int)rc);
        s_imu_publisher = rcl_get_zero_initialized_publisher();
        s_imu_ready = false;
    }
    if (s_wheel_ticks_ready && node != NULL) {
        rcl_ret_t rc = rcl_publisher_fini(&s_wheel_ticks_publisher, node);
        if (rc != RCL_RET_OK) ESP_LOGW(TAG, "wheel publisher cleanup failed: %d", (int)rc);
        s_wheel_ticks_publisher = rcl_get_zero_initialized_publisher();
        s_wheel_ticks_ready = false;
    }
    s_healthy = false;
}

bool ros_publishers_is_healthy(void)
{
    return s_healthy;
}
