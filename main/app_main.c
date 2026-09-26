#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "app_config/app_config.h"
#include "control/differential_controller.h"
#include "control/command_mux.h"
#include "control/motor_pid_controller.h"
#include "control/odometry_estimator.h"
#include "control/servo_controller.h"
#include "drivers/battery_monitor.h"
#include "drivers/buzzer.h"
#include "drivers/status_led.h"
#include "ros_interface/ros_executor.h"
#include "icm42670p.h"
#include "utils/telemetry_buffer.h"

static const char *TAG = "app_main";
static const int PID_LOG_PERIOD_MS = 100;
static const int LOW_VOLTAGE_BLINK_PERIOD_MS = 200;
static const int LOW_VOLTAGE_ALARM_DURATION_MS = 30000;
static const int STARTUP_OK_BEEP_MS = 120;
static bool s_imu_ready_for_telemetry = false;

static void app_enter_low_voltage_alarm(float voltage, const char *phase, bool stop_motion)
{
    ESP_LOGE(
        TAG,
        "%s battery voltage too low: %.2fV < %.2fV",
        phase,
        voltage,
        BATTERY_LOW_VOLTAGE_ENTER_V);

    if (stop_motion) {
        command_mux_set_motion_blocked(true);
    }

    status_led_on();
    buzzer_on();
    for (int elapsed_ms = 0;
         elapsed_ms < LOW_VOLTAGE_ALARM_DURATION_MS;
         elapsed_ms += LOW_VOLTAGE_BLINK_PERIOD_MS) {
        vTaskDelay(pdMS_TO_TICKS(LOW_VOLTAGE_BLINK_PERIOD_MS));
        status_led_toggle();
        buzzer_toggle();
    }

    buzzer_off();
    status_led_off();

    if (stop_motion) {
        /* Release the motor brake before sleeping to minimize external load. */
        motor_pid_controller_stop(false);
    }

    ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));
    esp_deep_sleep_start();
}

static void app_handle_startup_low_voltage(void)
{
    if (!battery_monitor_wait_ready(1000)) {
        ESP_LOGW(TAG, "battery voltage not ready during startup check");
        return;
    }

    const float startup_voltage = battery_monitor_get_voltage();

    if (!battery_monitor_is_low()) {
        buzzer_on();
        vTaskDelay(pdMS_TO_TICKS(STARTUP_OK_BEEP_MS));
        buzzer_off();
        return;
    }

    app_enter_low_voltage_alarm(startup_voltage, "startup", false);
}

static void app_log_pid_status(const char *phase)
{
    motor_pid_telemetry_t m1 = {0};
    motor_pid_telemetry_t m3 = {0};
    float gyro_rad_s[3] = {0};
    int imu_start_status = Icm42670p_Start_OK();

    motor_pid_controller_get_m1_telemetry(&m1);
    motor_pid_controller_get_m3_telemetry(&m3);

    if (imu_start_status > 0) {
        s_imu_ready_for_telemetry = true;
        Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
        telemetry_buffer_push(phase, &m1, &m3, gyro_rad_s[2], imu_start_status);
        ESP_LOGI(
            TAG,
            "%s | m1 target=%.2f actual=%.2f pwm=%d | m3 target=%.2f actual=%.2f pwm=%d | gyro_z=%.4f",
            phase,
            m1.target_rpm,
            m1.actual_rpm,
            m1.pwm_output,
            m3.target_rpm,
            m3.actual_rpm,
            m3.pwm_output,
            gyro_rad_s[2]);
        return;
    }

    if (s_imu_ready_for_telemetry) {
        telemetry_buffer_push(phase, &m1, &m3, 0.0f, imu_start_status);
    }

    ESP_LOGI(
        TAG,
        "%s | m1 target=%.2f actual=%.2f pwm=%d | m3 target=%.2f actual=%.2f pwm=%d | imu_status=%d",
        phase,
        m1.target_rpm,
        m1.actual_rpm,
        m1.pwm_output,
        m3.target_rpm,
        m3.actual_rpm,
        m3.pwm_output,
        imu_start_status);
}

void app_main(void)
{
    app_config_t *config = NULL;

    nvs_flash_init();
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("uros_transport", ESP_LOG_INFO);
    esp_log_level_set("ros_executor", ESP_LOG_INFO);
    esp_log_level_set("ros_publishers", ESP_LOG_INFO);

    status_led_init();
    buzzer_init();
    battery_monitor_init();
    app_handle_startup_low_voltage();

    app_config_init();
    config = app_config_get();

    servo_controller_init();
    servo_controller_set_center_offset(config->servo_center_offset_deg);
    servo_controller_center();

    Icm42670p_Init();

    differential_controller_init();
    command_mux_init();
    motor_pid_controller_set_pid(0.8f, 0.15f, 0.0f);
    odometry_estimator_init();
    telemetry_buffer_init();

    ros_executor_start();

    while (1) {
        if (battery_monitor_is_low()) {
            app_enter_low_voltage_alarm(battery_monitor_get_voltage(), "runtime", true);
        }
        app_log_pid_status("idle");
        vTaskDelay(pdMS_TO_TICKS(PID_LOG_PERIOD_MS));
    }
}
