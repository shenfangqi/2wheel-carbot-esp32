#include "control/motor_pid_controller.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "drivers/encoder_driver.h"
#include "drivers/motor_driver.h"

typedef struct {
    motor_id_t motor_id;
    encoder_id_t encoder_id;
    float target_rpm;
    float actual_rpm;
    float integral;
    float prev_error;
    int pwm_output;
    int last_encoder_count;
} motor_pid_channel_t;

static const char *TAG = "motor_pid";

static const TickType_t PID_PERIOD_TICKS = pdMS_TO_TICKS(10);
static const float PID_PERIOD_SEC = 0.01f;
static const float ENCODER_PULSES_PER_REV = 1040.0f;
static const float RPM_PER_PULSE = 60.0f / (ENCODER_PULSES_PER_REV * PID_PERIOD_SEC);
static const int MOTOR_PID_MAX_PWM = PWM_MOTOR_INPUT_MAX_VALUE;
static const float MOTOR_PID_MAX_INTEGRAL = 1000.0f;

static TaskHandle_t s_pid_task_handle = NULL;
static bool s_pid_initialized = false;
static bool s_pid_enabled = false;
static portMUX_TYPE s_pid_lock = portMUX_INITIALIZER_UNLOCKED;

static float s_kp = 1.0f;
static float s_ki = 0.2f;
static float s_kd = 0.2f;

static motor_pid_channel_t s_m1 = {
    .motor_id = MOTOR_ID_M1,
    .encoder_id = ENCODER_ID_M1,
};

static motor_pid_channel_t s_m3 = {
    .motor_id = MOTOR_ID_M3,
    .encoder_id = ENCODER_ID_M3,
};

static float motor_pid_clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static int motor_pid_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static int motor_pid_limit_output_direction(float target_rpm, int pwm_output)
{
    if (target_rpm > 0.0f && pwm_output < 0) {
        return 0;
    }

    if (target_rpm < 0.0f && pwm_output > 0) {
        return 0;
    }

    return pwm_output;
}

static void motor_pid_reset_channel(motor_pid_channel_t *channel, bool brake)
{
    channel->target_rpm = 0.0f;
    channel->actual_rpm = 0.0f;
    channel->integral = 0.0f;
    channel->prev_error = 0.0f;
    channel->pwm_output = 0;
    channel->last_encoder_count = encoder_driver_get_count(channel->encoder_id);
    motor_driver_stop(channel->motor_id, brake);
}

static void motor_pid_update_channel(motor_pid_channel_t *channel, float kp, float ki, float kd, bool enabled)
{
    int current_count = encoder_driver_get_count(channel->encoder_id);
    int delta_count = current_count - channel->last_encoder_count;
    float error = 0.0f;
    float derivative = 0.0f;
    float output = 0.0f;

    channel->last_encoder_count = current_count;
    channel->actual_rpm = (float)delta_count * RPM_PER_PULSE;

    if (!enabled || channel->target_rpm == 0.0f) {
        channel->integral = 0.0f;
        channel->prev_error = 0.0f;
        channel->pwm_output = 0;
        motor_driver_stop(channel->motor_id, true);
        return;
    }

    error = channel->target_rpm - channel->actual_rpm;
    channel->integral = motor_pid_clamp_float(
        channel->integral + (error * PID_PERIOD_SEC),
        -MOTOR_PID_MAX_INTEGRAL,
        MOTOR_PID_MAX_INTEGRAL);
    derivative = (error - channel->prev_error) / PID_PERIOD_SEC;
    output = (kp * error) + (ki * channel->integral) + (kd * derivative);

    channel->pwm_output = motor_pid_clamp_int(
        (int)output,
        -MOTOR_PID_MAX_PWM,
        MOTOR_PID_MAX_PWM);
    channel->pwm_output = motor_pid_limit_output_direction(channel->target_rpm, channel->pwm_output);
    channel->prev_error = error;

    if (channel->pwm_output == 0) {
        motor_driver_stop(channel->motor_id, true);
    } else {
        motor_driver_set_speed(channel->motor_id, channel->pwm_output);
    }
}

static void motor_pid_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "start motor PID task");

    s_m1.last_encoder_count = encoder_driver_get_count(s_m1.encoder_id);
    s_m3.last_encoder_count = encoder_driver_get_count(s_m3.encoder_id);

    TickType_t last_wake_time = xTaskGetTickCount();
    while (1) {
        float kp = 0.0f;
        float ki = 0.0f;
        float kd = 0.0f;
        bool enabled = false;

        portENTER_CRITICAL(&s_pid_lock);
        kp = s_kp;
        ki = s_ki;
        kd = s_kd;
        enabled = s_pid_enabled;
        portEXIT_CRITICAL(&s_pid_lock);

        motor_pid_update_channel(&s_m1, kp, ki, kd, enabled);
        motor_pid_update_channel(&s_m3, kp, ki, kd, enabled);

        vTaskDelayUntil(&last_wake_time, PID_PERIOD_TICKS);
    }
}

void motor_pid_controller_init(void)
{
    if (s_pid_initialized) {
        return;
    }

    encoder_driver_init();
    motor_driver_init();
    motor_driver_stop(MOTOR_ID_ALL, true);

    memset(&s_m1, 0, sizeof(s_m1));
    memset(&s_m3, 0, sizeof(s_m3));
    s_m1.motor_id = MOTOR_ID_M1;
    s_m1.encoder_id = ENCODER_ID_M1;
    s_m3.motor_id = MOTOR_ID_M3;
    s_m3.encoder_id = ENCODER_ID_M3;
    s_m1.last_encoder_count = encoder_driver_get_count(s_m1.encoder_id);
    s_m3.last_encoder_count = encoder_driver_get_count(s_m3.encoder_id);

    xTaskCreatePinnedToCore(motor_pid_task, "motor_pid_task", 4096, NULL, 10, &s_pid_task_handle, 1);
    s_pid_initialized = true;
}

void motor_pid_controller_set_pid(float kp, float ki, float kd)
{
    portENTER_CRITICAL(&s_pid_lock);
    s_kp = kp;
    s_ki = ki;
    s_kd = kd;
    portEXIT_CRITICAL(&s_pid_lock);
}

void motor_pid_controller_set_target_rpm(float m1_target_rpm, float m3_target_rpm)
{
    portENTER_CRITICAL(&s_pid_lock);
    s_m1.target_rpm = m1_target_rpm;
    s_m3.target_rpm = m3_target_rpm;
    s_pid_enabled = (m1_target_rpm != 0.0f) || (m3_target_rpm != 0.0f);
    portEXIT_CRITICAL(&s_pid_lock);
}

void motor_pid_controller_stop(bool brake)
{
    portENTER_CRITICAL(&s_pid_lock);
    s_pid_enabled = false;
    s_m1.target_rpm = 0.0f;
    s_m3.target_rpm = 0.0f;
    portEXIT_CRITICAL(&s_pid_lock);

    motor_pid_reset_channel(&s_m1, brake);
    motor_pid_reset_channel(&s_m3, brake);
}

static void motor_pid_copy_telemetry(const motor_pid_channel_t *channel, motor_pid_telemetry_t *out)
{
    if (out == NULL) {
        return;
    }

    out->target_rpm = channel->target_rpm;
    out->actual_rpm = channel->actual_rpm;
    out->pwm_output = channel->pwm_output;
}

void motor_pid_controller_get_m1_telemetry(motor_pid_telemetry_t *out)
{
    portENTER_CRITICAL(&s_pid_lock);
    motor_pid_copy_telemetry(&s_m1, out);
    portEXIT_CRITICAL(&s_pid_lock);
}

void motor_pid_controller_get_m3_telemetry(motor_pid_telemetry_t *out)
{
    portENTER_CRITICAL(&s_pid_lock);
    motor_pid_copy_telemetry(&s_m3, out);
    portEXIT_CRITICAL(&s_pid_lock);
}
