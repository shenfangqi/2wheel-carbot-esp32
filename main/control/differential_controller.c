#include "control/differential_controller.h"

#include <math.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "control/motor_pid_controller.h"
#include "control/safety_manager.h"
#include "icm42670p.h"

// Physical track-center spacing is 225 mm. Ground turning calibration uses a
// larger effective width to account for track scrub. Bidirectional ground
// tests refined the effective value to 254 mm.
static const float DIFFERENTIAL_TRACK_WIDTH_M = 0.254f;
// Empirical effective radius calibrated from two 1.025 m ground runs that each
// targeted 1.000 m using the provisional 21.25 mm radius.
static const float DIFFERENTIAL_SPROCKET_RADIUS_M = 0.02175f;
static const float DIFFERENTIAL_MAX_WHEEL_RPM = 250.0f;
// Ground tests bracketed the neutral point near 1.0. Apply the currently
// selected 0.05% correction without changing calibrated turning commands.
static const float DIFFERENTIAL_STRAIGHT_RIGHT_TRIM = 1.0005f;
/* Keep ordinary cmd_vel driving independent of IMU bias. Heading-assisted
 * calibration remains available through the explicit CLI test. */
static const bool DIFFERENTIAL_ENABLE_AUTOMATIC_HEADING_HOLD = false;
static const float DIFFERENTIAL_HEADING_KP = 1.5f;
static const float DIFFERENTIAL_YAW_RATE_KD = 0.15f;
static const float DIFFERENTIAL_MAX_HEADING_CORRECTION_RAD_S = 0.15f;
static const float PI_F = 3.14159265358979323846f;
static portMUX_TYPE s_differential_lock = portMUX_INITIALIZER_UNLOCKED;
static float s_requested_linear_mps = 0.0f;
static float s_requested_angular_rps = 0.0f;
static uint32_t s_command_generation = 0;
static differential_heading_status_t s_heading_status = {0};
static safety_limiter_t s_motion_limiter = {0};

static float differential_linear_to_rpm(float linear_mps)
{
    if (DIFFERENTIAL_SPROCKET_RADIUS_M <= 0.0f) {
        return 0.0f;
    }

    return (linear_mps * 60.0f) /
           (2.0f * PI_F * DIFFERENTIAL_SPROCKET_RADIUS_M);
}

static void differential_limit_pair(float *left_rpm, float *right_rpm)
{
    float max_magnitude = fmaxf(fabsf(*left_rpm), fabsf(*right_rpm));

    if (max_magnitude > DIFFERENTIAL_MAX_WHEEL_RPM) {
        float scale = DIFFERENTIAL_MAX_WHEEL_RPM / max_magnitude;
        *left_rpm *= scale;
        *right_rpm *= scale;
    }
}

static void differential_apply_cmd(float linear_mps, float angular_rps,
                                   bool apply_straight_trim)
{
    float half_track_m = DIFFERENTIAL_TRACK_WIDTH_M * 0.5f;
    float left_linear_mps = linear_mps - (angular_rps * half_track_m);
    float right_linear_mps = linear_mps + (angular_rps * half_track_m);
    float left_rpm = differential_linear_to_rpm(left_linear_mps);
    float right_rpm = differential_linear_to_rpm(right_linear_mps);

    if (apply_straight_trim && fabsf(linear_mps) > 0.0f) {
        right_rpm *= DIFFERENTIAL_STRAIGHT_RIGHT_TRIM;
    }

    differential_limit_pair(&left_rpm, &right_rpm);

    // M3 is the left track. On this chassis, vehicle-forward is positive for M1
    // and negative for M3.
    motor_pid_controller_set_target_rpm(right_rpm, -left_rpm);
}

static void differential_heading_task(void *arg)
{
    (void)arg;
    const int bias_sample_target = 100;
    int bias_samples = 0;
    float bias_sum = 0.0f;
    bool imu_calibrated = false;
    float gyro_z_bias_rad_s = 0.0f;
    float relative_yaw_rad = 0.0f;
    bool was_heading_active = false;
    int64_t last_imu_us = esp_timer_get_time();
    safety_manager_reset_limiter(&s_motion_limiter);
    safety_manager_limit_acceleration(&s_motion_limiter, 0.0f, 0.0f,
                                      last_imu_us, NULL, NULL);

    while (1) {
        float linear_mps = 0.0f;
        float angular_rps = 0.0f;
        uint32_t command_generation = 0;
        portENTER_CRITICAL(&s_differential_lock);
        linear_mps = s_requested_linear_mps;
        angular_rps = s_requested_angular_rps;
        command_generation = s_command_generation;
        portEXIT_CRITICAL(&s_differential_lock);

        const bool stop_requested = differential_command_is_stopped(linear_mps, angular_rps);
        if (stop_requested) {
            safety_manager_reset_limiter(&s_motion_limiter);
            safety_manager_limit_acceleration(&s_motion_limiter, 0.0f, 0.0f,
                                              esp_timer_get_time(), NULL, NULL);
        } else {
            safety_manager_limit_acceleration(&s_motion_limiter,
                                              linear_mps,
                                              angular_rps,
                                              esp_timer_get_time(),
                                              &linear_mps,
                                              &angular_rps);
        }

        bool stopped = fabsf(linear_mps) < 0.0001f && fabsf(angular_rps) < 0.0001f;
        bool straight = fabsf(linear_mps) > 0.0001f && fabsf(angular_rps) < 0.0001f;
        int imu_status = Icm42670p_Start_OK();
        float gyro_rad_s[3] = {0};

        if (imu_status > 0) {
            Icm42670p_Get_Gyro_rad_s(gyro_rad_s);
        }

        if (!imu_calibrated && imu_status > 0 && stopped) {
            bias_sum += gyro_rad_s[2];
            bias_samples++;
            if (bias_samples >= bias_sample_target) {
                gyro_z_bias_rad_s = bias_sum / bias_samples;
                imu_calibrated = true;
            }
        } else if (!stopped && !imu_calibrated) {
            bias_sum = 0.0f;
            bias_samples = 0;
        }

        bool heading_active = differential_heading_hold_should_activate(
            DIFFERENTIAL_ENABLE_AUTOMATIC_HEADING_HOLD,
            straight,
            imu_calibrated,
            imu_status > 0);
        float correction_rad_s = 0.0f;
        int64_t now_imu_us = esp_timer_get_time();

        if (heading_active) {
            if (!was_heading_active) {
                relative_yaw_rad = 0.0f;
                last_imu_us = now_imu_us;
            }
            float yaw_rate_rad_s = gyro_rad_s[2] - gyro_z_bias_rad_s;
            relative_yaw_rad += yaw_rate_rad_s *
                ((now_imu_us - last_imu_us) / 1000000.0f);
            correction_rad_s = DIFFERENTIAL_HEADING_KP * relative_yaw_rad +
                               DIFFERENTIAL_YAW_RATE_KD * yaw_rate_rad_s;
            if (correction_rad_s > DIFFERENTIAL_MAX_HEADING_CORRECTION_RAD_S) {
                correction_rad_s = DIFFERENTIAL_MAX_HEADING_CORRECTION_RAD_S;
            } else if (correction_rad_s < -DIFFERENTIAL_MAX_HEADING_CORRECTION_RAD_S) {
                correction_rad_s = -DIFFERENTIAL_MAX_HEADING_CORRECTION_RAD_S;
            }
        }
        last_imu_us = now_imu_us;
        was_heading_active = heading_active;

        portENTER_CRITICAL(&s_differential_lock);
        s_heading_status.imu_calibrated = imu_calibrated;
        s_heading_status.heading_active = heading_active;
        s_heading_status.gyro_z_bias_rad_s = gyro_z_bias_rad_s;
        s_heading_status.relative_yaw_rad = relative_yaw_rad;
        s_heading_status.correction_rad_s = correction_rad_s;
        portEXIT_CRITICAL(&s_differential_lock);

        /* A stop/new command may arrive while IMU and limiter work is being
         * calculated. Never apply a command computed from an older snapshot. */
        portENTER_CRITICAL(&s_differential_lock);
        bool command_is_current = differential_command_snapshot_is_current(
            command_generation, s_command_generation);
        portEXIT_CRITICAL(&s_differential_lock);

        if (!stopped && command_is_current) {
            differential_apply_cmd(linear_mps,
                                   heading_active ? correction_rad_s : angular_rps,
                                   straight);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void differential_controller_init(void)
{
    motor_pid_controller_init();
    xTaskCreate(differential_heading_task, "differential_heading", 3072,
                NULL, 8, NULL);
}

void differential_controller_set_cmd(float linear_mps, float angular_rps)
{
    portENTER_CRITICAL(&s_differential_lock);
    s_requested_linear_mps = linear_mps;
    s_requested_angular_rps = angular_rps;
    s_command_generation++;
    portEXIT_CRITICAL(&s_differential_lock);

    /* A zero command is safety-critical and must not wait for/ramp through the
     * periodic limiter. */
    if (differential_command_is_stopped(linear_mps, angular_rps)) {
        motor_pid_controller_stop(true);
    }

    /* The 20 ms heading task applies acceleration limits before wheel conversion. */
}

void differential_controller_stop(void)
{
    portENTER_CRITICAL(&s_differential_lock);
    s_requested_linear_mps = 0.0f;
    s_requested_angular_rps = 0.0f;
    s_command_generation++;
    s_heading_status.heading_active = false;
    s_heading_status.correction_rad_s = 0.0f;
    portEXIT_CRITICAL(&s_differential_lock);
    motor_pid_controller_stop(true);
}

int differential_controller_distance_to_counts(float distance_m)
{
    float circumference_m = 2.0f * PI_F * DIFFERENTIAL_SPROCKET_RADIUS_M;

    if (distance_m <= 0.0f || circumference_m <= 0.0f) {
        return 0;
    }

    return (int)((distance_m / circumference_m) *
                 MOTOR_ENCODER_COUNTS_PER_REV + 0.5f);
}

int differential_controller_turn_to_counts(float angle_rad)
{
    float track_distance_m = fabsf(angle_rad) * DIFFERENTIAL_TRACK_WIDTH_M * 0.5f;
    return differential_controller_distance_to_counts(track_distance_m);
}

void differential_controller_get_heading_status(differential_heading_status_t *out)
{
    if (out == NULL) {
        return;
    }
    portENTER_CRITICAL(&s_differential_lock);
    *out = s_heading_status;
    portEXIT_CRITICAL(&s_differential_lock);
}
