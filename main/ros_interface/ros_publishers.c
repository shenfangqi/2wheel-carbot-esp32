#include "ros_interface/ros_publishers.h"

#include "esp_err.h"

esp_err_t ros_publishers_init(rcl_node_t *node, rclc_executor_t *executor)
{
    (void)node;
    (void)executor;
    return ESP_OK;
}

void ros_publishers_fini(rcl_node_t *node, rclc_executor_t *executor)
{
    (void)node;
    (void)executor;
}
