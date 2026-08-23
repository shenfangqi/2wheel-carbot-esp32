#include "drivers/buzzer.h"

#include "driver/gpio.h"

void buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    buzzer_off();
}

void buzzer_on(void)
{
    gpio_set_level(BUZZER_GPIO, BUZZER_ACTIVE_LEVEL);
}

void buzzer_off(void)
{
    gpio_set_level(BUZZER_GPIO, !BUZZER_ACTIVE_LEVEL);
}

void buzzer_set(uint8_t state)
{
    if (state) {
        buzzer_on();
    } else {
        buzzer_off();
    }
}

void buzzer_toggle(void)
{
    int level = gpio_get_level(BUZZER_GPIO);
    gpio_set_level(BUZZER_GPIO, !level);
}
