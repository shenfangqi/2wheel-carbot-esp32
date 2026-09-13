#include "control/odometry_estimator.h"

#include <stdbool.h>

#include "esp_random.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

#include "control/wheel_ticks_math.h"
#include "drivers/encoder_driver.h"

static portMUX_TYPE s_ticks_lock = portMUX_INITIALIZER_UNLOCKED;
static wheel_ticks_snapshot_t s_ticks;
static int s_previous_m1;
static int s_previous_m3;
static bool s_initialized;
static bool s_task_started;

static void odometry_estimator_task(void *arg)
{
    (void)arg;
    while (1) {
        odometry_estimator_sample();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void odometry_estimator_init(void)
{
    if (s_task_started) {
        return;
    }
    const int m1 = encoder_driver_get_count_m1();
    const int m3 = encoder_driver_get_count_m3();

    portENTER_CRITICAL(&s_ticks_lock);
    s_ticks = (wheel_ticks_snapshot_t) {
        .device_stamp_us = (uint64_t)esp_timer_get_time(),
        .boot_id = esp_random(),
    };
    s_previous_m1 = m1;
    s_previous_m3 = m3;
    s_initialized = true;
    portEXIT_CRITICAL(&s_ticks_lock);

    ESP_ERROR_CHECK(xTaskCreate(odometry_estimator_task, "wheel_ticks", 2048,
                                NULL, 6, NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    s_task_started = true;
}

void odometry_estimator_sample(void)
{
    const int m1 = encoder_driver_get_count_m1();
    const int m3 = encoder_driver_get_count_m3();
    const uint64_t now_us = (uint64_t)esp_timer_get_time();

    portENTER_CRITICAL(&s_ticks_lock);
    if (s_initialized) {
        /* PID feedback confirms vehicle-forward is negative M1 and positive M3. */
        s_ticks.right_ticks = wheel_ticks_accumulate_signed(
            s_ticks.right_ticks, s_previous_m1, m1, -1);
        s_ticks.left_ticks = wheel_ticks_accumulate(s_ticks.left_ticks, s_previous_m3, m3);
        s_ticks.sequence++;
        s_ticks.device_stamp_us = now_us;
    }
    s_previous_m1 = m1;
    s_previous_m3 = m3;
    portEXIT_CRITICAL(&s_ticks_lock);
}

void odometry_estimator_get_wheel_ticks(wheel_ticks_snapshot_t *out)
{
    if (out == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_ticks_lock);
    *out = s_ticks;
    portEXIT_CRITICAL(&s_ticks_lock);
}
