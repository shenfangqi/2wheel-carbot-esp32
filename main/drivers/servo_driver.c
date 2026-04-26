#include "drivers/servo_driver.h"

#include "driver/mcpwm_prelude.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "servo_driver";

static mcpwm_timer_handle_t servo_timer = NULL;
static mcpwm_oper_handle_t servo_operator = NULL;
static mcpwm_cmpr_handle_t servo_comparator = NULL;
static mcpwm_gen_handle_t servo_generator = NULL;
static int servo_current_angle_deg = SERVO_DEFAULT_ANGLE_DEG;

static int servo_driver_limit_angle(int angle_deg)
{
    if (angle_deg > SERVO_MAX_ANGLE_DEG) return SERVO_MAX_ANGLE_DEG;
    if (angle_deg < SERVO_MIN_ANGLE_DEG) return SERVO_MIN_ANGLE_DEG;
    return angle_deg;
}

static uint32_t servo_driver_angle_to_compare(int angle_deg)
{
    angle_deg = servo_driver_limit_angle(angle_deg);

    return (uint32_t)(((angle_deg - SERVO_MIN_ANGLE_DEG) *
        (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US)) /
        (SERVO_MAX_ANGLE_DEG - SERVO_MIN_ANGLE_DEG) + SERVO_MIN_PULSEWIDTH_US);
}

void servo_driver_init(void)
{
    if (servo_timer != NULL) {
        return;
    }

    mcpwm_timer_config_t timer_config = {
        .group_id = SERVO_TIMER_GROUP_ID,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD_TICKS,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &servo_timer));

    mcpwm_operator_config_t operator_config = {
        .group_id = SERVO_TIMER_GROUP_ID,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &servo_operator));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(servo_operator, servo_timer));

    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(servo_operator, &comparator_config, &servo_comparator));

    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = SERVO_GPIO_S1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(servo_operator, &generator_config, &servo_generator));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        servo_generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        servo_generator,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP, servo_comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        servo_comparator, servo_driver_angle_to_compare(SERVO_DEFAULT_ANGLE_DEG)));
    ESP_ERROR_CHECK(mcpwm_timer_enable(servo_timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(servo_timer, MCPWM_TIMER_START_NO_STOP));

    servo_current_angle_deg = SERVO_DEFAULT_ANGLE_DEG;
    ESP_LOGI(TAG, "servo S1 init on GPIO%d", SERVO_GPIO_S1);
}

void servo_driver_set_angle(int angle_deg)
{
    if (servo_comparator == NULL) {
        ESP_LOGE(TAG, "servo not initialized");
        return;
    }

    angle_deg = servo_driver_limit_angle(angle_deg);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        servo_comparator, servo_driver_angle_to_compare(angle_deg)));
    servo_current_angle_deg = angle_deg;
}

int servo_driver_get_angle(void)
{
    return servo_current_angle_deg;
}
