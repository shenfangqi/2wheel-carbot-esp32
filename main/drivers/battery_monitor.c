#include "drivers/battery_monitor.h"

#include <stdint.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BATTERY_MONITOR_ADC_UNIT        ADC_UNIT_1
#define BATTERY_MONITOR_ADC_CHANNEL     ADC_CHANNEL_2
#define BATTERY_MONITOR_ADC_ATTEN       ADC_ATTEN_DB_12
#define BATTERY_MONITOR_SAMPLE_PERIOD_MS 100

static const char *TAG = "battery_monitor";

static adc_oneshot_unit_handle_t s_battery_handle;
static adc_cali_handle_t s_battery_cali_handle;
static bool s_battery_cali_enabled = false;
static bool s_battery_task_started = false;
static volatile bool s_battery_ready = false;
static volatile bool s_battery_low = false;
static volatile float s_battery_voltage = 0.0f;

static bool battery_monitor_adc_calibration_init(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_atten_t atten,
    adc_cali_handle_t *out_handle)
{
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, out_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "adc calibration enabled");
        return true;
    }

    ESP_LOGW(TAG, "adc calibration unavailable: %s", esp_err_to_name(ret));
    *out_handle = NULL;
    return false;
}

static void battery_monitor_update_voltage(int gpio_voltage_mv)
{
    s_battery_voltage = (gpio_voltage_mv / 1000.0f) * BATTERY_MONITOR_VOLTAGE_SCALE;

    if (s_battery_low) {
        if (s_battery_voltage >= BATTERY_LOW_VOLTAGE_RELEASE_V) {
            s_battery_low = false;
        }
    } else if (s_battery_voltage <= BATTERY_LOW_VOLTAGE_ENTER_V) {
        s_battery_low = true;
    }
}

static void battery_monitor_task(void *arg)
{
    int adc_raw = 0;
    int gpio_voltage_mv = 0;

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(
            s_battery_handle,
            BATTERY_MONITOR_ADC_CHANNEL,
            &adc_raw));

        if (s_battery_cali_enabled) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(
                s_battery_cali_handle,
                adc_raw,
                &gpio_voltage_mv));
        } else {
            gpio_voltage_mv = (adc_raw * 3300) / 4095;
        }

        battery_monitor_update_voltage(gpio_voltage_mv);
        s_battery_ready = true;
        vTaskDelay(pdMS_TO_TICKS(BATTERY_MONITOR_SAMPLE_PERIOD_MS));
    }
}

void battery_monitor_init(void)
{
    if (s_battery_task_started) {
        return;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BATTERY_MONITOR_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_battery_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = BATTERY_MONITOR_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        s_battery_handle,
        BATTERY_MONITOR_ADC_CHANNEL,
        &channel_config));

    s_battery_cali_enabled = battery_monitor_adc_calibration_init(
        BATTERY_MONITOR_ADC_UNIT,
        BATTERY_MONITOR_ADC_CHANNEL,
        BATTERY_MONITOR_ADC_ATTEN,
        &s_battery_cali_handle);

    xTaskCreatePinnedToCore(
        battery_monitor_task,
        "battery_monitor",
        3 * 1024,
        NULL,
        2,
        NULL,
        1);
    s_battery_task_started = true;
}

bool battery_monitor_wait_ready(uint32_t timeout_ms)
{
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (!s_battery_ready) {
        if (xTaskGetTickCount() >= deadline) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return true;
}

float battery_monitor_get_voltage(void)
{
    return s_battery_voltage;
}

bool battery_monitor_is_low(void)
{
    return s_battery_ready && s_battery_low;
}
