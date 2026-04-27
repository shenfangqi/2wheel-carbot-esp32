#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#include <rcl/rcl.h>
#include <rclc/executor.h>

esp_err_t ros_publishers_init(rcl_node_t *node, rclc_executor_t *executor);
void ros_publishers_fini(rcl_node_t *node, rclc_executor_t *executor);

#ifdef __cplusplus
}
#endif
