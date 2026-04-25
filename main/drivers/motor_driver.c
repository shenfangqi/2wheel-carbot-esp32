#include "drivers/motor_driver.h"

#include "esp_log.h"
#include "esp_err.h"
#include "bdc_motor.h"

static const char *TAG = "motor_driver";

static bdc_motor_handle_t motor_m1 = NULL;
static bdc_motor_handle_t motor_m2 = NULL;
static bdc_motor_handle_t motor_m3 = NULL;
static bdc_motor_handle_t motor_m4 = NULL;

static bool stop_brake = true;

static int motor_driver_limit_input_speed(int speed)
{
    if (speed > PWM_MOTOR_INPUT_MAX_VALUE) return PWM_MOTOR_INPUT_MAX_VALUE;
    if (speed < -PWM_MOTOR_INPUT_MAX_VALUE) return -PWM_MOTOR_INPUT_MAX_VALUE;
    return speed;
}

static int motor_driver_speed_to_duty(int speed)
{
    speed = motor_driver_limit_input_speed(speed);

    if (speed > 0) {
        return speed + PWM_MOTOR_DEAD_ZONE;
    }

    if (speed < 0) {
        return speed - PWM_MOTOR_DEAD_ZONE;
    }

    return 0;
}

static void motor_driver_init_m1(void)
{
    bdc_motor_config_t motor_config = {
        .pwm_freq_hz = PWM_MOTOR_FREQ_HZ,
        .pwma_gpio_num = PWM_GPIO_M1B,
        .pwmb_gpio_num = PWM_GPIO_M1A,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = PWM_MOTOR_TIMER_GROUP_ID_M1,
        .resolution_hz = PWM_MOTOR_TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor_m1));
    ESP_ERROR_CHECK(bdc_motor_enable(motor_m1));
}

static void motor_driver_init_m2(void)
{
    bdc_motor_config_t motor_config = {
        .pwm_freq_hz = PWM_MOTOR_FREQ_HZ,
        .pwma_gpio_num = PWM_GPIO_M2B,
        .pwmb_gpio_num = PWM_GPIO_M2A,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = PWM_MOTOR_TIMER_GROUP_ID_M2,
        .resolution_hz = PWM_MOTOR_TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor_m2));
    ESP_ERROR_CHECK(bdc_motor_enable(motor_m2));
}

static void motor_driver_init_m3(void)
{
    bdc_motor_config_t motor_config = {
        .pwm_freq_hz = PWM_MOTOR_FREQ_HZ,
        .pwma_gpio_num = PWM_GPIO_M3B,
        .pwmb_gpio_num = PWM_GPIO_M3A,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = PWM_MOTOR_TIMER_GROUP_ID_M3,
        .resolution_hz = PWM_MOTOR_TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor_m3));
    ESP_ERROR_CHECK(bdc_motor_enable(motor_m3));
}

static void motor_driver_init_m4(void)
{
    bdc_motor_config_t motor_config = {
        .pwm_freq_hz = PWM_MOTOR_FREQ_HZ,
        .pwma_gpio_num = PWM_GPIO_M4B,
        .pwmb_gpio_num = PWM_GPIO_M4A,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = PWM_MOTOR_TIMER_GROUP_ID_M4,
        .resolution_hz = PWM_MOTOR_TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor_m4));
    ESP_ERROR_CHECK(bdc_motor_enable(motor_m4));
}

static void motor_driver_set_speed_m1(int speed)
{
    speed = motor_driver_speed_to_duty(speed);

    if (speed > 0) {
        ESP_ERROR_CHECK(bdc_motor_forward(motor_m1));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m1, speed));
    } else if (speed < 0) {
        ESP_ERROR_CHECK(bdc_motor_reverse(motor_m1));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m1, -speed));
    } else {
        if (stop_brake) ESP_ERROR_CHECK(bdc_motor_brake(motor_m1));
        else ESP_ERROR_CHECK(bdc_motor_coast(motor_m1));
    }
}

static void motor_driver_set_speed_m2(int speed)
{
    speed = motor_driver_speed_to_duty(speed);

    if (speed > 0) {
        ESP_ERROR_CHECK(bdc_motor_forward(motor_m2));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m2, speed));
    } else if (speed < 0) {
        ESP_ERROR_CHECK(bdc_motor_reverse(motor_m2));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m2, -speed));
    } else {
        if (stop_brake) ESP_ERROR_CHECK(bdc_motor_brake(motor_m2));
        else ESP_ERROR_CHECK(bdc_motor_coast(motor_m2));
    }
}

static void motor_driver_set_speed_m3(int speed)
{
    speed = motor_driver_speed_to_duty(speed);

    if (speed > 0) {
        ESP_ERROR_CHECK(bdc_motor_forward(motor_m3));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m3, speed));
    } else if (speed < 0) {
        ESP_ERROR_CHECK(bdc_motor_reverse(motor_m3));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m3, -speed));
    } else {
        if (stop_brake) ESP_ERROR_CHECK(bdc_motor_brake(motor_m3));
        else ESP_ERROR_CHECK(bdc_motor_coast(motor_m3));
    }
}

static void motor_driver_set_speed_m4(int speed)
{
    speed = motor_driver_speed_to_duty(speed);

    if (speed > 0) {
        ESP_ERROR_CHECK(bdc_motor_forward(motor_m4));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m4, speed));
    } else if (speed < 0) {
        ESP_ERROR_CHECK(bdc_motor_reverse(motor_m4));
        ESP_ERROR_CHECK(bdc_motor_set_speed(motor_m4, -speed));
    } else {
        if (stop_brake) ESP_ERROR_CHECK(bdc_motor_brake(motor_m4));
        else ESP_ERROR_CHECK(bdc_motor_coast(motor_m4));
    }
}

void motor_driver_init(void)
{
    ESP_LOGI(TAG, "init motors");
    motor_driver_init_m1();
    motor_driver_init_m2();
    motor_driver_init_m3();
    motor_driver_init_m4();
}

void motor_driver_set_speed(motor_id_t motor_id, int speed)
{
    if (motor_id == MOTOR_ID_M1) motor_driver_set_speed_m1(speed);
    else if (motor_id == MOTOR_ID_M2) motor_driver_set_speed_m2(speed);
    else if (motor_id == MOTOR_ID_M3) motor_driver_set_speed_m3(speed);
    else if (motor_id == MOTOR_ID_M4) motor_driver_set_speed_m4(speed);
    else if (motor_id == MOTOR_ID_ALL) {
        motor_driver_set_speed_m1(speed);
        motor_driver_set_speed_m2(speed);
        motor_driver_set_speed_m3(speed);
        motor_driver_set_speed_m4(speed);
    }
}

void motor_driver_set_speed_all(int speed_1, int speed_2, int speed_3, int speed_4)
{
    motor_driver_set_speed_m1(speed_1);
    motor_driver_set_speed_m2(speed_2);
    motor_driver_set_speed_m3(speed_3);
    motor_driver_set_speed_m4(speed_4);
}

void motor_driver_stop(motor_id_t motor_id, bool brake)
{
    stop_brake = brake;

    if (brake) {
        if (motor_id == MOTOR_ID_M1) ESP_ERROR_CHECK(bdc_motor_brake(motor_m1));
        else if (motor_id == MOTOR_ID_M2) ESP_ERROR_CHECK(bdc_motor_brake(motor_m2));
        else if (motor_id == MOTOR_ID_M3) ESP_ERROR_CHECK(bdc_motor_brake(motor_m3));
        else if (motor_id == MOTOR_ID_M4) ESP_ERROR_CHECK(bdc_motor_brake(motor_m4));
        else if (motor_id == MOTOR_ID_ALL) {
            ESP_ERROR_CHECK(bdc_motor_brake(motor_m1));
            ESP_ERROR_CHECK(bdc_motor_brake(motor_m2));
            ESP_ERROR_CHECK(bdc_motor_brake(motor_m3));
            ESP_ERROR_CHECK(bdc_motor_brake(motor_m4));
        }
    } else {
        if (motor_id == MOTOR_ID_M1) ESP_ERROR_CHECK(bdc_motor_coast(motor_m1));
        else if (motor_id == MOTOR_ID_M2) ESP_ERROR_CHECK(bdc_motor_coast(motor_m2));
        else if (motor_id == MOTOR_ID_M3) ESP_ERROR_CHECK(bdc_motor_coast(motor_m3));
        else if (motor_id == MOTOR_ID_M4) ESP_ERROR_CHECK(bdc_motor_coast(motor_m4));
        else if (motor_id == MOTOR_ID_ALL) {
            ESP_ERROR_CHECK(bdc_motor_coast(motor_m1));
            ESP_ERROR_CHECK(bdc_motor_coast(motor_m2));
            ESP_ERROR_CHECK(bdc_motor_coast(motor_m3));
            ESP_ERROR_CHECK(bdc_motor_coast(motor_m4));
        }
    }
}
