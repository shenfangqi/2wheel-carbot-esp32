#include "network/uros_transport.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <uxr/client/transport.h>

#include "network/uros_serial_io.h"

#define UROS_UART_RX_BUFFER_SIZE 2048
#define UROS_UART_TX_BUFFER_SIZE 2048

static const char *TAG = "uros_transport";
static const uart_port_t s_uart_port = UROS_UART_PORT;
static portMUX_TYPE s_transport_lock = portMUX_INITIALIZER_UNLOCKED;
static uros_serial_state_t s_transport_state = UROS_SERIAL_CLOSED;

static void uros_transport_apply_event(uros_serial_event_t event)
{
    portENTER_CRITICAL(&s_transport_lock);
    s_transport_state = uros_serial_next_state(s_transport_state, event);
    portEXIT_CRITICAL(&s_transport_lock);
}

static bool uros_serial_open(struct uxrCustomTransport *transport)
{
    const uart_port_t port = *(const uart_port_t *)transport->args;
    const uart_config_t config = {
        .baud_rate = UROS_SERIAL_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uros_transport_apply_event(UROS_SERIAL_EVENT_OPEN_BEGIN);
    /* Cleanup from a failed/abandoned session must not poison the next open. */
    esp_err_t result = uart_driver_delete(port);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "UART%d stale driver cleanup failed: %s", port, esp_err_to_name(result));
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
        return false;
    }

    result = uart_param_config(port, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "UART%d parameter configuration failed: %s", port, esp_err_to_name(result));
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
        return false;
    }

    result = uart_set_pin(port, UROS_UART_TX_GPIO, UROS_UART_RX_GPIO,
                          UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "UART%d GPIO routing TX=%d RX=%d failed: %s",
                 port, UROS_UART_TX_GPIO, UROS_UART_RX_GPIO, esp_err_to_name(result));
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
        return false;
    }

    result = uart_driver_install(port, UROS_UART_RX_BUFFER_SIZE,
                                 UROS_UART_TX_BUFFER_SIZE, 0, NULL, 0);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "UART%d driver install failed: %s", port, esp_err_to_name(result));
        uart_driver_delete(port);
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
        return false;
    }

    result = uart_flush_input(port);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "UART%d input flush failed: %s", port, esp_err_to_name(result));
        uart_driver_delete(port);
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
        return false;
    }

    ESP_LOGI(TAG, "UART%d ready: TX=%d RX=%d baud=%d 8N1 no-flow-control",
             port, UROS_UART_TX_GPIO, UROS_UART_RX_GPIO, UROS_SERIAL_BAUD_RATE);
    uros_transport_apply_event(UROS_SERIAL_EVENT_OPEN_OK);
    return true;
}

static bool uros_serial_close(struct uxrCustomTransport *transport)
{
    const uart_port_t port = *(const uart_port_t *)transport->args;
    esp_err_t result = uart_driver_delete(port);
    uros_transport_apply_event(UROS_SERIAL_EVENT_CLOSE);
    return result == ESP_OK || result == ESP_ERR_INVALID_STATE;
}

static size_t uros_serial_write(struct uxrCustomTransport *transport,
                                const uint8_t *buffer, size_t length,
                                uint8_t *error)
{
    const uart_port_t port = *(const uart_port_t *)transport->args;
    int written = uart_write_bytes(port, buffer, length);
    size_t result = uros_serial_io_result(written, length, error);
    if (written < 0) {
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
    }
    return result;
}

static size_t uros_serial_read(struct uxrCustomTransport *transport,
                               uint8_t *buffer, size_t length, int timeout_ms,
                               uint8_t *error)
{
    const uart_port_t port = *(const uart_port_t *)transport->args;
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ms > 0 && timeout_ticks == 0) {
        timeout_ticks = 1;
    }
    int read = uart_read_bytes(port, buffer, length, timeout_ticks);
    size_t result = uros_serial_io_result(read, length, error);
    if (read < 0) {
        uros_transport_apply_event(UROS_SERIAL_EVENT_IO_ERROR);
    }
    return result;
}

esp_err_t uros_transport_init(rmw_init_options_t *rmw_options)
{
    if (rmw_options == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
#if defined(RMW_UXRCE_TRANSPORT_CUSTOM)
    rmw_ret_t result = rmw_uros_options_set_custom_transport(
            UROS_SERIAL_FRAMING_ENABLED, (void *)&s_uart_port,
            uros_serial_open, uros_serial_close,
            uros_serial_write, uros_serial_read, rmw_options);
    return result == RMW_RET_OK ? ESP_OK : ESP_FAIL;
#else
#error "micro-ROS must be built with RMW_UXRCE_TRANSPORT=custom"
#endif
}

bool uros_transport_is_open(void)
{
    bool open;
    portENTER_CRITICAL(&s_transport_lock);
    open = s_transport_state == UROS_SERIAL_OPEN;
    portEXIT_CRITICAL(&s_transport_lock);
    return open;
}
