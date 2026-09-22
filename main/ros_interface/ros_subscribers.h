#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include <rcl/rcl.h>
#include <rclc/executor.h>

esp_err_t ros_subscribers_init(rcl_node_t *node, rclc_executor_t *executor);
void ros_subscribers_fini(rcl_node_t *node, rclc_executor_t *executor);
esp_err_t ros_subscribers_start_watchdog(void);
void ros_subscribers_reset_watchdog(void);

#ifdef __cplusplus
}
#endif
