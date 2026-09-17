#include "app_config/usb_config_cli.h"
#include "app_config/app_config.h"
#include "app_config/config_store.h"
#include "control/motor_pid_controller.h"
#include "control/differential_controller.h"
#include "drivers/battery_monitor.h"
#include "drivers/encoder_driver.h"
#include "network/wifi_manager.h"
#include "icm42670p.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_system.h"

#define CLI_UART_NUM        UART_NUM_0
#define CLI_RX_BUF_SIZE     256
#define CLI_TX_BUF_SIZE     0
#define CLI_LINE_BUF_SIZE   128

#define MOTOR1_TEST_RPM          (-50.0f)
#define MOTOR1_TEST_DURATION_MS  5000
#define MOTOR3_TEST_RPM          (50.0f)
#define MOTOR3_TEST_DURATION_MS  5000
#define MOTOR_DUAL_TEST_DURATION_MS 5000

static TaskHandle_t s_motor1_test_task = NULL;
static TaskHandle_t s_motor3_test_task = NULL;
static TaskHandle_t s_motor_stair_test_task = NULL;
static TaskHandle_t s_motor_dual_test_task = NULL;
static TaskHandle_t s_track_test_task = NULL;
static TaskHandle_t s_track_distance_test_task = NULL;
static TaskHandle_t s_track_turn_test_task = NULL;
static TaskHandle_t s_imu_static_test_task = NULL;

static bool motor_test_is_running(void)
{
    return s_motor1_test_task != NULL || s_motor3_test_task != NULL ||
           s_motor_stair_test_task != NULL || s_motor_dual_test_task != NULL ||
           s_track_test_task != NULL || s_track_distance_test_task != NULL ||
           s_track_turn_test_task != NULL || s_imu_static_test_task != NULL;
}

static void imu_static_test_task(void *arg)
{
    (void)arg;
    const int sample_count = 500;
    double gyro_mean[3] = {0};
    double gyro_m2[3] = {0};
    float gyro_min[3] = {0};
    float gyro_max[3] = {0};
    double accel_mean[3] = {0};

    if (Icm42670p_Start_OK() <= 0) {
        printf("\nimu static test failed; status=%d\n", Icm42670p_Start_OK());
        s_imu_static_test_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    for (int sample = 0; sample < sample_count; ++sample) {
        float gyro_rad_s[3] = {0};
        float accel_m_s2[3] = {0};
        Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
        Icm42670p_Get_Accel_m_s2(accel_m_s2);

        for (int axis = 0; axis < 3; ++axis) {
            double delta = gyro_rad_s[axis] - gyro_mean[axis];
            gyro_mean[axis] += delta / (sample + 1);
            gyro_m2[axis] += delta * (gyro_rad_s[axis] - gyro_mean[axis]);
            accel_mean[axis] += (accel_m_s2[axis] - accel_mean[axis]) / (sample + 1);
            if (sample == 0 || gyro_rad_s[axis] < gyro_min[axis]) gyro_min[axis] = gyro_rad_s[axis];
            if (sample == 0 || gyro_rad_s[axis] > gyro_max[axis]) gyro_max[axis] = gyro_rad_s[axis];
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("\nimu static test complete; samples=%d status=%d\n", sample_count,
           Icm42670p_Start_OK());
    for (int axis = 0; axis < 3; ++axis) {
        double stddev = sqrt(gyro_m2[axis] / (sample_count - 1));
        printf("gyro_%c_rad_s mean=%.6f std=%.6f min=%.6f max=%.6f\n",
               'x' + axis, gyro_mean[axis], stddev,
               (double)gyro_min[axis], (double)gyro_max[axis]);
    }
    printf("accel_m_s2 mean_x=%.4f mean_y=%.4f mean_z=%.4f\n",
           accel_mean[0], accel_mean[1], accel_mean[2]);
    s_imu_static_test_task = NULL;
    vTaskDelete(NULL);
}

static void motor1_test_task(void *arg)
{
    (void)arg;

    motor_pid_controller_set_target_rpm(MOTOR1_TEST_RPM, 0.0f);
    vTaskDelay(pdMS_TO_TICKS(MOTOR1_TEST_DURATION_MS));
    motor_pid_controller_stop(true);
    printf("\nmotor1 test complete; braked\n");
    s_motor1_test_task = NULL;
    vTaskDelete(NULL);
}

static void motor3_test_task(void *arg)
{
    (void)arg;

    motor_pid_controller_set_target_rpm(0.0f, MOTOR3_TEST_RPM);
    vTaskDelay(pdMS_TO_TICKS(MOTOR3_TEST_DURATION_MS));
    motor_pid_controller_stop(true);
    printf("\nmotor3 test complete; braked\n");
    s_motor3_test_task = NULL;
    vTaskDelete(NULL);
}

static void motor_stair_test_task(void *arg)
{
    const int motor_id = (int)(intptr_t)arg;
    const int pwm_steps[] = {1, 20, 40, 60};
    const int duty_percent[] = {60, 65, 70, 75};

    for (size_t i = 0; i < sizeof(pwm_steps) / sizeof(pwm_steps[0]); ++i) {
        motor_pid_telemetry_t telemetry = {0};
        int pwm = motor_id == 1 ? -pwm_steps[i] : pwm_steps[i];

        if (motor_id == 1) {
            motor_pid_controller_set_open_loop(pwm, 0);
        } else {
            motor_pid_controller_set_open_loop(0, pwm);
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        if (motor_id == 1) {
            motor_pid_controller_get_m1_telemetry(&telemetry);
        } else {
            motor_pid_controller_get_m3_telemetry(&telemetry);
        }
        printf("\nmotor%d stair duty=%d%% pwm=%d actual=%.2f rpm\n",
               motor_id, duty_percent[i], pwm, (double)telemetry.actual_rpm);
        motor_pid_controller_stop(true);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    printf("motor%d stair test complete; braked\n", motor_id);
    s_motor_stair_test_task = NULL;
    vTaskDelete(NULL);
}

static void motor_dual_test_task(void *arg)
{
    (void)arg;

    motor_pid_controller_set_target_rpm(MOTOR1_TEST_RPM, MOTOR3_TEST_RPM);
    vTaskDelay(pdMS_TO_TICKS(MOTOR_DUAL_TEST_DURATION_MS));
    motor_pid_controller_stop(true);
    printf("\ndual motor test complete; braked\n");
    s_motor_dual_test_task = NULL;
    vTaskDelete(NULL);
}

typedef enum {
    TRACK_TEST_FORWARD = 1,
    TRACK_TEST_BACKWARD,
    TRACK_TEST_LEFT,
    TRACK_TEST_RIGHT,
} track_test_t;

static void track_test_task(void *arg)
{
    track_test_t test = (track_test_t)(intptr_t)arg;

    if (test == TRACK_TEST_FORWARD) {
        differential_controller_set_cmd(0.10f, 0.0f);
    } else if (test == TRACK_TEST_BACKWARD) {
        differential_controller_set_cmd(-0.10f, 0.0f);
    } else if (test == TRACK_TEST_LEFT) {
        differential_controller_set_cmd(0.0f, 1.0f);
    } else {
        differential_controller_set_cmd(0.0f, -1.0f);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    differential_controller_stop();
    printf("\ntrack test complete; braked\n");
    s_track_test_task = NULL;
    vTaskDelete(NULL);
}

static void track_distance_test_task(void *arg)
{
    const bool use_imu_heading = (bool)(intptr_t)arg;
    const int target_counts = differential_controller_distance_to_counts(1.0f);
    const int mismatch_limit = target_counts / 5;
    const TickType_t timeout_ticks = pdMS_TO_TICKS(15000);
    const int m1_start = encoder_driver_get_count(ENCODER_ID_M1);
    const int m3_start = encoder_driver_get_count(ENCODER_ID_M3);
    TickType_t start_ticks = 0;
    bool completed = false;
    bool mismatch_abort = false;
    int m1_counts = 0;
    int m3_counts = 0;
    float gyro_z_bias_rad_s = 0.0f;
    float imu_yaw_rad = 0.0f;
    float max_correction_rad_s = 0.0f;

    if (use_imu_heading) {
        if (Icm42670p_Start_OK() <= 0) {
            printf("\ntrack 1m imu test aborted_imu; status=%d; braked\n",
                   Icm42670p_Start_OK());
            differential_controller_stop();
            s_track_distance_test_task = NULL;
            vTaskDelete(NULL);
            return;
        }
        for (int sample = 0; sample < 100; ++sample) {
            float gyro_rad_s[3] = {0};
            Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
            gyro_z_bias_rad_s += gyro_rad_s[2] / 100.0f;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    differential_controller_set_cmd(0.10f, 0.0f);
    start_ticks = xTaskGetTickCount();
    int64_t last_imu_us = esp_timer_get_time();

    while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
        if (use_imu_heading) {
            const float heading_kp = 1.5f;
            const float yaw_rate_kd = 0.15f;
            const float max_correction = 0.15f;
            float gyro_rad_s[3] = {0};
            int64_t now_imu_us = esp_timer_get_time();
            Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
            float yaw_rate_rad_s = gyro_rad_s[2] - gyro_z_bias_rad_s;
            imu_yaw_rad += yaw_rate_rad_s *
                           ((now_imu_us - last_imu_us) / 1000000.0f);
            last_imu_us = now_imu_us;

            // IMU Z is positive for a physical right turn on this chassis;
            // positive controller angular velocity commands a left turn.
            float correction_rad_s = heading_kp * imu_yaw_rad +
                                     yaw_rate_kd * yaw_rate_rad_s;
            if (correction_rad_s > max_correction) correction_rad_s = max_correction;
            if (correction_rad_s < -max_correction) correction_rad_s = -max_correction;
            if (fabsf(correction_rad_s) > max_correction_rad_s) {
                max_correction_rad_s = fabsf(correction_rad_s);
            }
            differential_controller_set_cmd(0.10f, correction_rad_s);
        }

        m1_counts = abs(encoder_driver_get_count(ENCODER_ID_M1) - m1_start);
        m3_counts = abs(encoder_driver_get_count(ENCODER_ID_M3) - m3_start);

        if (((m1_counts + m3_counts) / 2) >= target_counts) {
            completed = true;
            break;
        }

        if ((m1_counts + m3_counts) > 1000 && abs(m1_counts - m3_counts) > mismatch_limit) {
            mismatch_abort = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    differential_controller_stop();
    const char *result = completed ? "complete" :
                         (mismatch_abort ? "aborted_mismatch" : "aborted_timeout");
    if (use_imu_heading) {
        printf("\ntrack 1m imu test %s; m1_counts=%d m3_counts=%d target=%d; "
               "imu_yaw_deg=%.2f gyro_z_bias_rad_s=%.6f "
               "max_correction_rad_s=%.4f; braked\n",
               result, m1_counts, m3_counts, target_counts,
               (double)(imu_yaw_rad * 57.29577951308232f),
               (double)gyro_z_bias_rad_s, (double)max_correction_rad_s);
    } else {
        printf("\ntrack 1m test %s; m1_counts=%d m3_counts=%d target=%d; braked\n",
               result, m1_counts, m3_counts, target_counts);
    }
    s_track_distance_test_task = NULL;
    vTaskDelete(NULL);
}

static void track_turn_test_task(void *arg)
{
    const int signed_angle_deg = (int)(intptr_t)arg;
    const int direction = signed_angle_deg >= 0 ? 1 : -1;
    const int angle_deg = abs(signed_angle_deg);
    const float angle_rad = angle_deg * 0.01745329251994329577f;
    const int target_counts = differential_controller_turn_to_counts(angle_rad);
    const int mismatch_limit = target_counts / 5;
    const TickType_t timeout_ticks = pdMS_TO_TICKS(12000);
    const int m1_start = encoder_driver_get_count(ENCODER_ID_M1);
    const int m3_start = encoder_driver_get_count(ENCODER_ID_M3);
    const TickType_t start_ticks = xTaskGetTickCount();
    bool completed = false;
    bool mismatch_abort = false;
    int m1_counts = 0;
    int m3_counts = 0;
    float gyro_z_bias_rad_s = 0.0f;
    float imu_yaw_rad = 0.0f;

    if (Icm42670p_Start_OK() <= 0) {
        printf("\ntrack %s %d test aborted_imu; status=%d; braked\n",
               direction > 0 ? "left" : "right", angle_deg,
               Icm42670p_Start_OK());
        differential_controller_stop();
        s_track_turn_test_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    for (int sample = 0; sample < 100; ++sample) {
        float gyro_rad_s[3] = {0};
        Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
        gyro_z_bias_rad_s += gyro_rad_s[2] / 100.0f;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    differential_controller_set_cmd(0.0f, direction * 0.8f);
    int64_t last_imu_us = esp_timer_get_time();

    while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
        float gyro_rad_s[3] = {0};
        int64_t now_imu_us = esp_timer_get_time();
        Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
        imu_yaw_rad += (gyro_rad_s[2] - gyro_z_bias_rad_s) *
                       ((now_imu_us - last_imu_us) / 1000000.0f);
        last_imu_us = now_imu_us;
        m1_counts = abs(encoder_driver_get_count(ENCODER_ID_M1) - m1_start);
        m3_counts = abs(encoder_driver_get_count(ENCODER_ID_M3) - m3_start);

        if (((m1_counts + m3_counts) / 2) >= target_counts) {
            completed = true;
            break;
        }

        if ((m1_counts + m3_counts) > 1000 && abs(m1_counts - m3_counts) > mismatch_limit) {
            mismatch_abort = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    differential_controller_stop();
    printf("\ntrack %s %d test %s; m1_counts=%d m3_counts=%d target=%d; "
           "imu_yaw_deg=%.2f gyro_z_bias_rad_s=%.6f; braked\n",
           direction > 0 ? "left" : "right",
           angle_deg,
           completed ? "complete" : (mismatch_abort ? "aborted_mismatch" : "aborted_timeout"),
           m1_counts, m3_counts, target_counts,
           (double)(imu_yaw_rad * 57.29577951308232f),
           (double)gyro_z_bias_rad_s);
    s_track_turn_test_task = NULL;
    vTaskDelete(NULL);
}

static void print_prompt(void)
{
    printf("carbot> ");
    fflush(stdout);
}

static void handle_command(char *line)
{
    app_config_t *cfg = app_config_get();

    if (strcmp(line, "show") == 0) {
        printf("wifi_ssid=%s\n", cfg->wifi_ssid);
        printf("wifi_password=%s\n", cfg->wifi_password);
        printf("local_ip=%s\n", wifi_manager_get_ip());
        printf("agent_ip=%s\n", cfg->agent_ip);
        printf("agent_port=%d\n", cfg->agent_port);
    }
    else if (strncmp(line, "set ", 4) == 0) {
        char key[32] = {0};
        char value[64] = {0};

        if (sscanf(line + 4, "%31s %63s", key, value) == 2) {
            if (strcmp(key, "wifi_ssid") == 0) {
                strncpy(cfg->wifi_ssid, value, sizeof(cfg->wifi_ssid) - 1);
                printf("OK\n");
            } else if (strcmp(key, "wifi_password") == 0) {
                strncpy(cfg->wifi_password, value, sizeof(cfg->wifi_password) - 1);
                printf("OK\n");
            } else if (strcmp(key, "agent_ip") == 0) {
                strncpy(cfg->agent_ip, value, sizeof(cfg->agent_ip) - 1);
                printf("OK\n");
            } else if (strcmp(key, "agent_port") == 0) {
                cfg->agent_port = atoi(value);
                printf("OK\n");
            } else {
                printf("ERR unknown key\n");
            }
        } else {
            printf("ERR usage: set <key> <value>\n");
        }
    }
    else if (strcmp(line, "save") == 0) {
        config_store_save(cfg);
        printf("saved\n");
    }
    else if (strcmp(line, "reboot") == 0) {
        printf("rebooting...\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }
    else if (strcmp(line, "motor1_test") == 0) {
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(motor1_test_task, "motor1_test", 2048, NULL, 8,
                               &s_motor1_test_task) == pdPASS) {
            printf("motor1 test: %.0f rpm for %d ms; auto-brake armed\n",
                   (double)MOTOR1_TEST_RPM, MOTOR1_TEST_DURATION_MS);
        } else {
            s_motor1_test_task = NULL;
            motor_pid_controller_stop(true);
            printf("ERR failed to start motor1 test\n");
        }
    }
    else if (strcmp(line, "motor3_test") == 0) {
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(motor3_test_task, "motor3_test", 2048, NULL, 8,
                               &s_motor3_test_task) == pdPASS) {
            printf("motor3 test: %.0f rpm for %d ms; auto-brake armed\n",
                   (double)MOTOR3_TEST_RPM, MOTOR3_TEST_DURATION_MS);
        } else {
            s_motor3_test_task = NULL;
            motor_pid_controller_stop(true);
            printf("ERR failed to start motor3 test\n");
        }
    }
    else if (strcmp(line, "motor1_stair") == 0 || strcmp(line, "motor3_stair") == 0) {
        int motor_id = line[5] == '1' ? 1 : 3;
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(motor_stair_test_task, "motor_stair", 3072,
                               (void *)(intptr_t)motor_id, 8,
                               &s_motor_stair_test_task) == pdPASS) {
            printf("motor%d open-loop stair test 60/65/70/75%%; auto-brake armed\n",
                   motor_id);
        } else {
            s_motor_stair_test_task = NULL;
            motor_pid_controller_stop(true);
            printf("ERR failed to start motor stair test\n");
        }
    }
    else if (strcmp(line, "motor_dual_test") == 0) {
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(motor_dual_test_task, "motor_dual_test", 2048,
                               NULL, 8, &s_motor_dual_test_task) == pdPASS) {
            printf("dual motor test: 50 rpm for %d ms; auto-brake armed\n",
                   MOTOR_DUAL_TEST_DURATION_MS);
        } else {
            s_motor_dual_test_task = NULL;
            motor_pid_controller_stop(true);
            printf("ERR failed to start dual motor test\n");
        }
    }
    else if (strcmp(line, "track_forward") == 0 ||
             strcmp(line, "track_backward") == 0 ||
             strcmp(line, "track_left") == 0 ||
             strcmp(line, "track_right") == 0) {
        track_test_t test = 0;

        if (strcmp(line, "track_forward") == 0) test = TRACK_TEST_FORWARD;
        else if (strcmp(line, "track_backward") == 0) test = TRACK_TEST_BACKWARD;
        else if (strcmp(line, "track_left") == 0) test = TRACK_TEST_LEFT;
        else if (strcmp(line, "track_right") == 0) test = TRACK_TEST_RIGHT;

        if (test == 0) {
            printf("ERR usage: track_forward|track_backward|track_left|track_right\n");
        } else if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(track_test_task, "track_test", 2048,
                               (void *)(intptr_t)test, 8,
                               &s_track_test_task) == pdPASS) {
            printf("track test started for 500 ms; auto-brake armed\n");
        } else {
            s_track_test_task = NULL;
            differential_controller_stop();
            printf("ERR failed to start track test\n");
        }
    }
    else if (strcmp(line, "track_1m") == 0 || strcmp(line, "track_1m_imu") == 0) {
        bool use_imu_heading = strcmp(line, "track_1m_imu") == 0;
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(track_distance_test_task, "track_1m", 3072,
                               (void *)(intptr_t)use_imu_heading, 8,
                               &s_track_distance_test_task) == pdPASS) {
            printf("track 1m%s test started; 0.10 m/s; auto-brake and 15s timeout armed\n",
                   use_imu_heading ? " imu" : "");
        } else {
            s_track_distance_test_task = NULL;
            differential_controller_stop();
            printf("ERR failed to start track 1m test\n");
        }
    }
    else if (strcmp(line, "track_turn_360") == 0 ||
             strcmp(line, "track_turn_right_360") == 0 ||
             strcmp(line, "track_turn_left_90") == 0 ||
             strcmp(line, "track_turn_right_90") == 0) {
        int signed_angle_deg = 360;
        if (strcmp(line, "track_turn_right_360") == 0) signed_angle_deg = -360;
        else if (strcmp(line, "track_turn_left_90") == 0) signed_angle_deg = 90;
        else if (strcmp(line, "track_turn_right_90") == 0) signed_angle_deg = -90;
        if (motor_test_is_running()) {
            printf("ERR motor test already running\n");
        } else if (xTaskCreate(track_turn_test_task, "track_turn_360", 3072,
                               (void *)(intptr_t)signed_angle_deg, 8,
                               &s_track_turn_test_task) == pdPASS) {
            printf("track %s %d test started; 0.8 rad/s; auto-brake and 12s timeout armed\n",
                   signed_angle_deg > 0 ? "left" : "right", abs(signed_angle_deg));
        } else {
            s_track_turn_test_task = NULL;
            differential_controller_stop();
            printf("ERR failed to start track 360 test\n");
        }
    }
    else if (strcmp(line, "imu_static_test") == 0) {
        if (motor_test_is_running()) {
            printf("ERR motor or imu test already running\n");
        } else if (xTaskCreate(imu_static_test_task, "imu_static_test", 3072,
                               NULL, 7, &s_imu_static_test_task) == pdPASS) {
            printf("imu static test started; keep vehicle still for 5 seconds\n");
        } else {
            s_imu_static_test_task = NULL;
            printf("ERR failed to start imu static test\n");
        }
    }
    else if (strcmp(line, "motor_stop") == 0) {
        motor_pid_controller_stop(true);
        printf("motors braked\n");
    }
    else if (strcmp(line, "motor_status") == 0) {
        motor_pid_telemetry_t m1 = {0};
        motor_pid_telemetry_t m3 = {0};
        motor_pid_controller_get_m1_telemetry(&m1);
        motor_pid_controller_get_m3_telemetry(&m3);
        printf("battery=%.2fV low=%d m1_target=%.2f m1_actual=%.2f m1_pwm=%d "
               "m3_target=%.2f m3_actual=%.2f m3_pwm=%d\n",
               (double)battery_monitor_get_voltage(), battery_monitor_is_low(),
               (double)m1.target_rpm, (double)m1.actual_rpm, m1.pwm_output,
               (double)m3.target_rpm, (double)m3.actual_rpm, m3.pwm_output);
    }
    else if (strcmp(line, "heading_status") == 0) {
        differential_heading_status_t status = {0};
        differential_controller_get_heading_status(&status);
        printf("imu_calibrated=%d heading_active=%d gyro_z_bias_rad_s=%.6f "
               "relative_yaw_deg=%.3f correction_rad_s=%.4f\n",
               status.imu_calibrated, status.heading_active,
               (double)status.gyro_z_bias_rad_s,
               (double)(status.relative_yaw_rad * 57.29577951308232f),
               (double)status.correction_rad_s);
    }
    else if (strcmp(line, "encoder_counts") == 0) {
        printf("encoder_m1=%d encoder_m3=%d\n",
               encoder_driver_get_count(ENCODER_ID_M1),
               encoder_driver_get_count(ENCODER_ID_M3));
    }
    else if (strlen(line) == 0) {
        // 空行不处理
    }
    else {
        printf("ERR unknown command\n");
    }
}

static void cli_task(void *arg)
{
    uint8_t ch;
    char line[CLI_LINE_BUF_SIZE];
    int idx = 0;

    printf("uart cli ready\n");
    print_prompt();

    while (1) {
        int len = uart_read_bytes(CLI_UART_NUM, &ch, 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            if (ch == '\r' || ch == '\n') {
                printf("\n");
                line[idx] = '\0';
                handle_command(line);
                idx = 0;
                memset(line, 0, sizeof(line));
                print_prompt();
            } else if (ch == 0x08 || ch == 0x7F) {
                if (idx > 0) {
                    idx--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else {
                if (idx < CLI_LINE_BUF_SIZE - 1) {
                    line[idx++] = (char)ch;
                    printf("%c", ch);
                    fflush(stdout);
                }
            }
        }
    }
}

void usb_cli_start(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION_MAJOR >= 5
        .source_clk = UART_SCLK_DEFAULT,
#endif
    };

    uart_driver_install(CLI_UART_NUM, CLI_RX_BUF_SIZE, CLI_TX_BUF_SIZE, 0, NULL, 0);
    uart_param_config(CLI_UART_NUM, &uart_config);

    xTaskCreate(cli_task, "cli_task", 4096, NULL, 5, NULL);
}
