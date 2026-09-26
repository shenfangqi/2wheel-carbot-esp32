#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_config/app_config.h"
#include "control/battery_safety_policy.h"
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
static const int STARTUP_OK_BEEP_MS = 120;
static bool s_imu_ready_for_telemetry = false;
static battery_safety_state_t s_battery_safety_state = BATTERY_SAFETY_NORMAL;
static TickType_t s_low_voltage_last_toggle_tick;

static void app_update_battery_safety(void)
{
    const bool battery_low = battery_monitor_is_low();
    battery_safety_transition_t transition =
        battery_safety_update(&s_battery_safety_state, battery_low);

    if (transition == BATTERY_SAFETY_ENTER_ALARM) {
        ESP_LOGE(TAG, "battery low/disconnected: %.2fV; motion blocked",
                 battery_monitor_get_voltage());
        command_mux_set_motion_blocked(true);
        status_led_on();
        buzzer_on();
        s_low_voltage_last_toggle_tick = xTaskGetTickCount();
    } else if (transition == BATTERY_SAFETY_EXIT_ALARM) {
        ESP_LOGI(TAG, "battery recovered: %.2fV; new command required",
                 battery_monitor_get_voltage());
        buzzer_off();
        status_led_off();
        command_mux_set_motion_blocked(false);
    }

    if (s_battery_safety_state == BATTERY_SAFETY_ALARM &&
        xTaskGetTickCount() - s_low_voltage_last_toggle_tick >=
            pdMS_TO_TICKS(LOW_VOLTAGE_BLINK_PERIOD_MS)) {
        status_led_toggle();
        buzzer_toggle();
        s_low_voltage_last_toggle_tick = xTaskGetTickCount();
    }
}

static void app_signal_healthy_startup(void)
{
    if (!battery_monitor_wait_ready(1000)) {
        ESP_LOGW(TAG, "battery voltage not ready during startup check");
        return;
    }

    if (battery_monitor_is_low()) {
        ESP_LOGW(TAG, "startup battery low/disconnected: %.2fV; continuing in blocked mode",
                 battery_monitor_get_voltage());
        return;
    }

    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(STARTUP_OK_BEEP_MS));
    buzzer_off();
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
    app_signal_healthy_startup();

    app_config_init();
    config = app_config_get();

    servo_controller_init();
    servo_controller_set_center_offset(config->servo_center_offset_deg);
    servo_controller_center();

    Icm42670p_Init();

    differential_controller_init();
    command_mux_init();
    app_update_battery_safety();
    motor_pid_controller_set_pid(0.8f, 0.15f, 0.0f);
    odometry_estimator_init();
    telemetry_buffer_init();

    ros_executor_start();

    while (1) {
        app_update_battery_safety();
        app_log_pid_status("idle");
        vTaskDelay(pdMS_TO_TICKS(PID_LOG_PERIOD_MS));
    }
}
