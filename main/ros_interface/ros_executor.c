#include "ros_executor.h"

#include <inttypes.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>

#include "control/command_mux.h"
#include "network/uros_transport.h"
#include "network/wifi_manager.h"
#include "ros_interface/ros_publishers.h"
#include "ros_interface/ros_health.h"
#include "ros_interface/ros_subscribers.h"
#include "ros_interface/ros_topics.h"
#include "ros_interface/ros_time.h"

static const char *TAG = "ros_executor";
static TaskHandle_t s_ros_task_handle = NULL;
static uint32_t s_reconnect_count = 0;
static ros_disconnect_reason_t s_last_disconnect_reason = ROS_DISCONNECT_NONE;
static uint32_t s_consecutive_ping_failures = 0;
static int64_t s_session_started_us = 0;
static uint64_t s_last_session_uptime_ms = 0;
static bool s_session_active = false;

static const char *ros_executor_disconnect_reason_name(ros_disconnect_reason_t reason)
{
    switch (reason) {
    case ROS_DISCONNECT_WIFI: return "wifi";
    case ROS_DISCONNECT_EXECUTOR: return "executor";
    case ROS_DISCONNECT_PUBLISHER: return "publisher";
    case ROS_DISCONNECT_AGENT_PING: return "agent_ping";
    case ROS_DISCONNECT_ENTITY_INIT: return "entity_init";
    case ROS_DISCONNECT_NONE:
    default: return "none";
    }
}

static void ros_executor_log_rcl_ret(const char *op, rcl_ret_t rc)
{
    if (rc != RCL_RET_OK) {
        ESP_LOGW(TAG, "%s failed during cleanup: %d", op, (int)rc);
    }
}

static void ros_executor_stop_ros_motion(void)
{
    ros_subscribers_reset_watchdog();
    command_mux_stop_ros(true);
}

static esp_err_t ros_executor_connect_support(rclc_support_t *support, rcl_allocator_t *allocator)
{
    rcl_ret_t rc = RCL_RET_OK;
    rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();

    if (support == NULL || allocator == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rc = rcl_init_options_init(&init_options, *allocator);
    if (rc != RCL_RET_OK) {
        ESP_LOGE(TAG, "rcl init options failed: %d", (int)rc);
        return ESP_FAIL;
    }

    rc = rcl_init_options_set_domain_id(&init_options, CONFIG_CARBOT_MICRO_ROS_DOMAIN_ID);
    if (rc != RCL_RET_OK) {
        ESP_LOGE(TAG, "set domain id failed: %d", (int)rc);
        ros_executor_log_rcl_ret("rcl_init_options_fini", rcl_init_options_fini(&init_options));
        return ESP_FAIL;
    }

    if (uros_transport_init(rcl_init_options_get_rmw_init_options(&init_options)) != ESP_OK) {
        ESP_LOGE(TAG, "configure UDP transport failed");
        ros_executor_log_rcl_ret("rcl_init_options_fini", rcl_init_options_fini(&init_options));
        return ESP_FAIL;
    }

    if (rmw_uros_ping_agent_options(
            250, 3, rcl_init_options_get_rmw_init_options(&init_options)) != RMW_RET_OK) {
        ESP_LOGW(TAG, "micro-ROS agent ping failed");
        ros_executor_log_rcl_ret("rcl_init_options_fini", rcl_init_options_fini(&init_options));
        return ESP_FAIL;
    }

    rc = rclc_support_init_with_options(support, 0, NULL, &init_options, allocator);
    ros_executor_log_rcl_ret("rcl_init_options_fini", rcl_init_options_fini(&init_options));
    if (rc != RCL_RET_OK) {
        ESP_LOGW(
            TAG,
            "agent connect failed: %d (%s:%s)",
            (int)rc,
            uros_transport_get_agent_ip(),
            uros_transport_get_agent_port());
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void ros_executor_task(void *arg)
{
    (void)arg;

    while (1) {
        rcl_allocator_t allocator = rcl_get_default_allocator();
        rclc_support_t support = {0};
        rcl_node_t node = rcl_get_zero_initialized_node();
        rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
        bool support_ready = false;
        bool node_ready = false;
        bool executor_ready = false;

        if (!wifi_manager_is_connected()) {
            s_last_disconnect_reason = ROS_DISCONNECT_WIFI;
            ros_executor_stop_ros_motion();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (ros_executor_connect_support(&support, &allocator) != ESP_OK) {
            s_last_disconnect_reason = ROS_DISCONNECT_ENTITY_INIT;
            ros_executor_stop_ros_motion();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        support_ready = true;

        ESP_LOGI(
            TAG,
            "connected to micro-ROS agent %s:%s",
            uros_transport_get_agent_ip(),
            uros_transport_get_agent_port());

        if (rclc_node_init_default(&node, ROS_NODE_NAME, CONFIG_CARBOT_MICRO_ROS_NAMESPACE, &support) != RCL_RET_OK) {
            ESP_LOGE(TAG, "node init failed");
            s_last_disconnect_reason = ROS_DISCONNECT_ENTITY_INIT;
            goto cleanup;
        }
        node_ready = true;

        ros_time_reset();
        if (!ros_time_sync(1000)) {
            ESP_LOGW(TAG, "micro-ROS epoch synchronization failed");
        }

        if (rclc_executor_init(&executor, &support.context, 2, &allocator) != RCL_RET_OK) {
            ESP_LOGE(TAG, "executor init failed");
            s_last_disconnect_reason = ROS_DISCONNECT_ENTITY_INIT;
            goto cleanup;
        }
        executor_ready = true;

        if (ros_publishers_init(&node, &support, &executor) != ESP_OK) {
            ESP_LOGE(TAG, "publisher init failed");
            s_last_disconnect_reason = ROS_DISCONNECT_ENTITY_INIT;
            goto cleanup;
        }

        if (ros_subscribers_init(&node, &executor) != ESP_OK) {
            ESP_LOGE(TAG, "subscriber init failed");
            s_last_disconnect_reason = ROS_DISCONNECT_ENTITY_INIT;
            goto cleanup;
        }
        s_reconnect_count++;
        s_consecutive_ping_failures = 0;
        s_session_started_us = esp_timer_get_time();
        s_session_active = true;

        int64_t last_health_check_ms = s_session_started_us / 1000;
        int64_t last_time_sync_ms = last_health_check_ms;
        while (wifi_manager_is_connected()) {
            rcl_ret_t rc = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(CONFIG_CARBOT_MICRO_ROS_SPIN_PERIOD_MS));
            if (rc != RCL_RET_OK) {
                ESP_LOGE(TAG, "executor spin failed: %d", (int)rc);
                s_last_disconnect_reason = ROS_DISCONNECT_EXECUTOR;
                break;
            }
            if (!ros_publishers_is_healthy()) {
                ESP_LOGE(TAG, "publisher health check failed");
                s_last_disconnect_reason = ROS_DISCONNECT_PUBLISHER;
                break;
            }

            int64_t now_ms = esp_timer_get_time() / 1000;
            if (ros_health_check_due(
                    now_ms,
                    last_health_check_ms,
                    CONFIG_CARBOT_MICRO_ROS_HEALTH_CHECK_INTERVAL_MS)) {
                last_health_check_ms = now_ms;
                bool ping_succeeded = rmw_uros_ping_agent(
                    CONFIG_CARBOT_MICRO_ROS_HEALTH_CHECK_TIMEOUT_MS,
                    CONFIG_CARBOT_MICRO_ROS_HEALTH_CHECK_ATTEMPTS) == RMW_RET_OK;
                if (ros_health_record_ping_result(
                        &s_consecutive_ping_failures,
                        ping_succeeded,
                        CONFIG_CARBOT_MICRO_ROS_HEALTH_CHECK_FAILURE_LIMIT)) {
                    s_last_disconnect_reason = ROS_DISCONNECT_AGENT_PING;
                    ESP_LOGE(
                        TAG,
                        "agent health check failed %" PRIu32 " consecutive rounds",
                        s_consecutive_ping_failures);
                    break;
                }
            }
            if (ros_health_check_due(
                    now_ms,
                    last_time_sync_ms,
                    CONFIG_CARBOT_MICRO_ROS_TIME_SYNC_INTERVAL_MS)) {
                last_time_sync_ms = now_ms;
                if (!ros_time_sync(CONFIG_CARBOT_MICRO_ROS_TIME_SYNC_TIMEOUT_MS)) {
                    ESP_LOGW(TAG, "periodic micro-ROS epoch synchronization failed");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        if (!wifi_manager_is_connected()) {
            s_last_disconnect_reason = ROS_DISCONNECT_WIFI;
        }

cleanup:
        if (s_session_active) {
            s_last_session_uptime_ms =
                (uint64_t)((esp_timer_get_time() - s_session_started_us) / 1000);
            s_session_active = false;
            ESP_LOGE(
                TAG,
                "micro-ROS session ended: reason=%s uptime_ms=%" PRIu64,
                ros_executor_disconnect_reason_name(s_last_disconnect_reason),
                s_last_session_uptime_ms);
        }
        ros_executor_stop_ros_motion();
        ros_subscribers_fini(&node, &executor);
        ros_publishers_fini(&node, &executor);

        if (executor_ready) {
            ros_executor_log_rcl_ret("rclc_executor_fini", rclc_executor_fini(&executor));
        }
        if (node_ready) {
            ros_executor_log_rcl_ret("rcl_node_fini", rcl_node_fini(&node));
        }
        if (support_ready) {
            ros_executor_log_rcl_ret("rclc_support_fini", rclc_support_fini(&support));
        }
        ros_time_reset();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

uint32_t ros_executor_get_reconnect_count(void)
{
    return s_reconnect_count > 0 ? s_reconnect_count - 1 : 0;
}

ros_disconnect_reason_t ros_executor_get_last_disconnect_reason(void)
{
    return s_last_disconnect_reason;
}

uint32_t ros_executor_get_consecutive_ping_failures(void)
{
    return s_consecutive_ping_failures;
}

uint64_t ros_executor_get_session_uptime_ms(void)
{
    if (s_session_active) {
        return (uint64_t)((esp_timer_get_time() - s_session_started_us) / 1000);
    }
    return s_last_session_uptime_ms;
}

void ros_executor_start(void)
{
    if (!CONFIG_CARBOT_MICRO_ROS_ENABLED) {
        printf("ros executor disabled\n");
        return;
    }

    if (s_ros_task_handle != NULL) {
        return;
    }

    if (ros_subscribers_start_watchdog() != ESP_OK) {
        printf("ros cmd_vel watchdog start failed\n");
        return;
    }

    printf("ros executor start\n");
    xTaskCreate(
        ros_executor_task,
        "micro_ros_task",
        CONFIG_CARBOT_MICRO_ROS_TASK_STACK,
        NULL,
        CONFIG_CARBOT_MICRO_ROS_TASK_PRIO,
        &s_ros_task_handle);
}
