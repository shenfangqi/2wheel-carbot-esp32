#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

#define UROS_UART_PORT UART_NUM_0
#define UROS_UART_TX_GPIO GPIO_NUM_43
#define UROS_UART_RX_GPIO GPIO_NUM_44

esp_err_t uros_transport_init(rmw_init_options_t *rmw_options);
bool uros_transport_is_open(void);
