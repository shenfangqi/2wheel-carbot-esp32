#include "uros_transport.h"
#include "app_config.h"
#include <stdio.h>

void uros_transport_init(void)
{
    app_config_t* cfg = app_config_get();

    printf("uros init\n");
    printf("agent: %s:%d\n", cfg->agent_ip, cfg->agent_port);

    // TODO: micro-ROS 初始化
}