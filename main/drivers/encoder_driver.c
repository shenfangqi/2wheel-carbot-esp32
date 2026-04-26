#include "drivers/encoder_driver.h"

#include "driver/pulse_cnt.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "encoder_driver";

static pcnt_unit_handle_t encoder_unit_m1 = NULL;
static pcnt_unit_handle_t encoder_unit_m2 = NULL;
static pcnt_unit_handle_t encoder_unit_m3 = NULL;
static pcnt_unit_handle_t encoder_unit_m4 = NULL;

static void encoder_driver_init_unit(pcnt_unit_handle_t *out_unit, int gpio_a, int gpio_b)
{
    pcnt_unit_config_t unit_config = {
        .high_limit = ENCODER_PCNT_HIGH_LIMIT,
        .low_limit = ENCODER_PCNT_LOW_LIMIT,
        .flags.accum_count = true,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, out_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = ENCODER_GLITCH_FILTER_NS,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(*out_unit, &filter_config));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = gpio_a,
        .level_gpio_num = gpio_b,
    };
    pcnt_channel_handle_t chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(*out_unit, &chan_a_config, &chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = gpio_b,
        .level_gpio_num = gpio_a,
    };
    pcnt_channel_handle_t chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(*out_unit, &chan_b_config, &chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_a,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_b,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(*out_unit, ENCODER_PCNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(*out_unit, ENCODER_PCNT_LOW_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_enable(*out_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(*out_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(*out_unit));
}

static int encoder_driver_get_count_from_unit(pcnt_unit_handle_t unit)
{
    int count = 0;

    if (unit == NULL) {
        return 0;
    }

    ESP_ERROR_CHECK(pcnt_unit_get_count(unit, &count));
    return count;
}

void encoder_driver_init(void)
{
    ESP_LOGI(TAG, "init encoder PCNT units");

    encoder_driver_init_unit(&encoder_unit_m1, ENCODER_GPIO_H1A, ENCODER_GPIO_H1B);
    encoder_driver_init_unit(&encoder_unit_m2, ENCODER_GPIO_H2A, ENCODER_GPIO_H2B);
    encoder_driver_init_unit(&encoder_unit_m3, ENCODER_GPIO_H3B, ENCODER_GPIO_H3A);
    encoder_driver_init_unit(&encoder_unit_m4, ENCODER_GPIO_H4B, ENCODER_GPIO_H4A);
}

int encoder_driver_get_count_m1(void)
{
    return encoder_driver_get_count_from_unit(encoder_unit_m1);
}

int encoder_driver_get_count_m2(void)
{
    return encoder_driver_get_count_from_unit(encoder_unit_m2);
}

int encoder_driver_get_count_m3(void)
{
    return -encoder_driver_get_count_from_unit(encoder_unit_m3);
}

int encoder_driver_get_count_m4(void)
{
    return encoder_driver_get_count_from_unit(encoder_unit_m4);
}

int encoder_driver_get_count(encoder_id_t encoder_id)
{
    if (encoder_id == ENCODER_ID_M1) return encoder_driver_get_count_m1();
    if (encoder_id == ENCODER_ID_M2) return encoder_driver_get_count_m2();
    if (encoder_id == ENCODER_ID_M3) return encoder_driver_get_count_m3();
    if (encoder_id == ENCODER_ID_M4) return encoder_driver_get_count_m4();
    return 0;
}
