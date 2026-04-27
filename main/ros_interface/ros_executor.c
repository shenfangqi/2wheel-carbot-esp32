#include "ros_executor.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

#include "control/command_mux.h"
#include "network/uros_transport.h"
#include "network/wifi_manager.h"
#include "ros_interface/ros_publishers.h"
#include "ros_interface/ros_subscribers.h"
#include "ros_interface/ros_topics.h"

static const char *TAG = "ros_executor";
static TaskHandle_t s_ros_task_handle = NULL;

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
            ros_executor_stop_ros_motion();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (ros_executor_connect_support(&support, &allocator) != ESP_OK) {
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
            goto cleanup;
        }
        node_ready = true;

        if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
            ESP_LOGE(TAG, "executor init failed");
            goto cleanup;
        }
        executor_ready = true;

        if (ros_publishers_init(&node, &executor) != ESP_OK) {
            ESP_LOGE(TAG, "publisher init failed");
            goto cleanup;
        }

        if (ros_subscribers_init(&node, &executor) != ESP_OK) {
            ESP_LOGE(TAG, "subscriber init failed");
            goto cleanup;
        }

        while (wifi_manager_is_connected()) {
            rcl_ret_t rc = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(CONFIG_CARBOT_MICRO_ROS_SPIN_PERIOD_MS));
            ros_subscribers_check_timeout(CONFIG_CARBOT_MICRO_ROS_CMD_VEL_TIMEOUT_MS);
            if (rc != RCL_RET_OK) {
                ESP_LOGE(TAG, "executor spin failed: %d", (int)rc);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

cleanup:
        ros_executor_stop_ros_motion();
        ros_publishers_fini(&node, &executor);
        ros_subscribers_fini(&node, &executor);

        if (executor_ready) {
            ros_executor_log_rcl_ret("rclc_executor_fini", rclc_executor_fini(&executor));
        }
        if (node_ready) {
            ros_executor_log_rcl_ret("rcl_node_fini", rcl_node_fini(&node));
        }
        if (support_ready) {
            ros_executor_log_rcl_ret("rclc_support_fini", rclc_support_fini(&support));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
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

    printf("ros executor start\n");
    xTaskCreate(
        ros_executor_task,
        "micro_ros_task",
        CONFIG_CARBOT_MICRO_ROS_TASK_STACK,
        NULL,
        CONFIG_CARBOT_MICRO_ROS_TASK_PRIO,
        &s_ros_task_handle);
}
