#include "drivers/status_led.h"

#include "driver/gpio.h"

void status_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    status_led_off();
}

void status_led_on(void)
{
    gpio_set_level(STATUS_LED_GPIO, STATUS_LED_ACTIVE_LEVEL);
}

void status_led_off(void)
{
    gpio_set_level(STATUS_LED_GPIO, !STATUS_LED_ACTIVE_LEVEL);
}

void status_led_set(uint8_t state)
{
    if (state) {
        status_led_on();
    } else {
        status_led_off();
    }
}

void status_led_toggle(void)
{
    int level = gpio_get_level(STATUS_LED_GPIO);
    gpio_set_level(STATUS_LED_GPIO, !level);
}
