#pragma once

#include "esp_err.h"

#include <rmw_microros/rmw_microros.h>

esp_err_t uros_transport_init(rmw_init_options_t *rmw_options);
const char *uros_transport_get_agent_ip(void);
const char *uros_transport_get_agent_port(void);
