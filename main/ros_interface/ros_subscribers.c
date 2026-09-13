#include "ros_interface/ros_subscribers.h"

#include <inttypes.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <geometry_msgs/msg/twist.h>
#include <rosidl_runtime_c/message_type_support_struct.h>
#include <rclc/subscription.h>

#include "control/command_mux.h"
#include "control/safety_manager.h"
#include "ros_interface/ros_topics.h"

static const char *TAG = "ros_subscribers";

static rcl_subscription_t s_cmd_vel_subscriber;
static geometry_msgs__msg__Twist s_cmd_vel_msg;
static bool s_subscriber_initialized = false;
static int64_t s_last_cmd_vel_ms = -1;

static int64_t ros_subscribers_now_ms(void)
{
    return esp_timer_get_time() / 1000LL;
}

static void ros_subscribers_log_rcl_ret(const char *op, rcl_ret_t rc)
{
    if (rc != RCL_RET_OK) {
        ESP_LOGW(TAG, "%s failed during cleanup: %d", op, (int)rc);
    }
}

static void ros_subscribers_cmd_vel_callback(const void *msgin)
{
    const geometry_msgs__msg__Twist *msg = (const geometry_msgs__msg__Twist *)msgin;

    if (msg == NULL) {
        return;
    }

    if (!safety_manager_validate((float)msg->linear.x, (float)msg->angular.z)) {
        command_mux_apply_ros_cmd((float)msg->linear.x, (float)msg->angular.z);
        ESP_LOGE(TAG, "rejected invalid cmd_vel");
        return;
    }
    s_last_cmd_vel_ms = ros_subscribers_now_ms();
    ESP_LOGI(TAG, "cmd_vel linear=%.3f angular=%.3f", (float)msg->linear.x, (float)msg->angular.z);
    command_mux_apply_ros_cmd((float)msg->linear.x, (float)msg->angular.z);
}

esp_err_t ros_subscribers_init(rcl_node_t *node, rclc_executor_t *executor)
{
    rcl_ret_t rc = RCL_RET_OK;

    if (node == NULL || executor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_cmd_vel_subscriber = rcl_get_zero_initialized_subscription();
    s_cmd_vel_msg = (geometry_msgs__msg__Twist){0};
    s_last_cmd_vel_ms = -1;

    rc = rclc_subscription_init_default(
        &s_cmd_vel_subscriber,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        ROS_TOPIC_CMD_VEL);
    if (rc != RCL_RET_OK) {
        ESP_LOGE(TAG, "cmd_vel subscriber init failed: %d", (int)rc);
        return ESP_FAIL;
    }

    rc = rclc_executor_add_subscription(
        executor,
        &s_cmd_vel_subscriber,
        &s_cmd_vel_msg,
        &ros_subscribers_cmd_vel_callback,
        ON_NEW_DATA);
    if (rc != RCL_RET_OK) {
        ESP_LOGE(TAG, "cmd_vel executor add failed: %d", (int)rc);
        ros_subscribers_log_rcl_ret("rcl_subscription_fini", rcl_subscription_fini(&s_cmd_vel_subscriber, node));
        s_cmd_vel_subscriber = rcl_get_zero_initialized_subscription();
        return ESP_FAIL;
    }

    s_subscriber_initialized = true;
    return ESP_OK;
}

void ros_subscribers_fini(rcl_node_t *node, rclc_executor_t *executor)
{
    (void)executor;

    if (!s_subscriber_initialized || node == NULL) {
        return;
    }

    ros_subscribers_log_rcl_ret("rcl_subscription_fini", rcl_subscription_fini(&s_cmd_vel_subscriber, node));
    s_cmd_vel_subscriber = rcl_get_zero_initialized_subscription();
    s_last_cmd_vel_ms = -1;
    s_subscriber_initialized = false;
}

void ros_subscribers_reset_watchdog(void)
{
    s_last_cmd_vel_ms = -1;
}

void ros_subscribers_check_timeout(int32_t timeout_ms)
{
    int64_t now_ms = 0;

    if (!s_subscriber_initialized || timeout_ms <= 0 || s_last_cmd_vel_ms < 0) {
        return;
    }

    now_ms = ros_subscribers_now_ms();
    if ((now_ms - s_last_cmd_vel_ms) < timeout_ms) {
        return;
    }

    ESP_LOGW(TAG, "cmd_vel timeout after %" PRId64 " ms", now_ms - s_last_cmd_vel_ms);
    s_last_cmd_vel_ms = -1;
    command_mux_stop_ros(true);
}
