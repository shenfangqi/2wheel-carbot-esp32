#include "uros_transport.h"

#include <stdio.h>

#include <rcl/rcl.h>

#include "app_config/app_config.h"

static char s_agent_port[16];

esp_err_t uros_transport_init(rmw_init_options_t *rmw_options)
{
    app_config_t *cfg = app_config_get();
    rmw_ret_t ret = RMW_RET_ERROR;

    if (cfg == NULL || rmw_options == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(s_agent_port, sizeof(s_agent_port), "%d", cfg->agent_port);

    printf("uros init\n");
    printf("agent: %s:%s\n", cfg->agent_ip, s_agent_port);

    ret = rmw_uros_options_set_udp_address(cfg->agent_ip, s_agent_port, rmw_options);
    if (ret != RMW_RET_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

const char *uros_transport_get_agent_ip(void)
{
    app_config_t *cfg = app_config_get();

    if (cfg == NULL) {
        return "";
    }

    return cfg->agent_ip;
}

const char *uros_transport_get_agent_port(void)
{
    return s_agent_port;
}
