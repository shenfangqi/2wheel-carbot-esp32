#pragma once

#include "esp_err.h"

#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

esp_err_t uros_transport_init(rmw_init_options_t *rmw_options);
bool uros_transport_is_open(void);
