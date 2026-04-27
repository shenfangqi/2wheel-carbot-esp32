#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_config/app_config.h"
#include "app_config/usb_config_cli.h"
#include "network/wifi_manager.h"
#include "control/ackermann_controller.h"
#include "control/command_mux.h"
#include "control/motor_pid_controller.h"
#include "control/servo_controller.h"
#include "ros_interface/ros_executor.h"
#include "icm42670p.h"
#include "utils/telemetry_buffer.h"

static const char *TAG = "app_main";
static const int PID_LOG_PERIOD_MS = 100;
static bool s_imu_ready_for_telemetry = false;

static void app_log_pid_status(const char *phase)
{
    motor_pid_telemetry_t m1 = {0};
    motor_pid_telemetry_t m3 = {0};
    float gyro_dps[3] = {0};
    int imu_start_status = Icm42670p_Start_OK();

    motor_pid_controller_get_m1_telemetry(&m1);
    motor_pid_controller_get_m3_telemetry(&m3);

    if (imu_start_status > 0) {
        s_imu_ready_for_telemetry = true;
        Icm42670p_Get_Gyro_dps(gyro_dps);
        telemetry_buffer_push(phase, &m1, &m3, gyro_dps[2], imu_start_status);
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
            gyro_dps[2]);
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

    printf("=== CARBOT START ===\n");

    app_config_init();
    config = app_config_get();
    printf("config init ok\n");

    usb_cli_start();
    printf("cli start ok\n");

    servo_controller_init();
    servo_controller_set_center_offset(config->servo_center_offset_deg);
    servo_controller_center();
    printf("servo init ok\n");

    ackermann_controller_init();
    printf("ackermann controller init ok\n");
    command_mux_init();
    printf("command mux init ok\n");
    motor_pid_controller_set_pid(0.8f, 0.05f, 0.0f);
    printf("motor pid init ok\n");
    telemetry_buffer_init();
    printf("telemetry buffer init ok\n");

    wifi_manager_init();
    printf("wifi manager init ok\n");

    ros_executor_start();
    printf("ros executor init ok\n");

    Icm42670p_Init();
    printf("imu init start\n");

    while (1) {
        app_log_pid_status("idle");
        vTaskDelay(pdMS_TO_TICKS(PID_LOG_PERIOD_MS));
    }
}
