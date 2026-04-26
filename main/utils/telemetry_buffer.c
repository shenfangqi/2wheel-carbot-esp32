#include "utils/telemetry_buffer.h"

#include <string.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define TELEMETRY_BUFFER_CAPACITY 256

static telemetry_sample_t s_samples[TELEMETRY_BUFFER_CAPACITY];
static size_t s_head = 0;
static size_t s_count = 0;
static uint32_t s_sequence = 0;
static bool s_initialized = false;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

void telemetry_buffer_init(void)
{
    portENTER_CRITICAL(&s_lock);
    memset(s_samples, 0, sizeof(s_samples));
    s_head = 0;
    s_count = 0;
    s_sequence = 0;
    s_initialized = true;
    portEXIT_CRITICAL(&s_lock);
}

bool telemetry_buffer_initialized(void)
{
    bool initialized = false;

    portENTER_CRITICAL(&s_lock);
    initialized = s_initialized;
    portEXIT_CRITICAL(&s_lock);

    return initialized;
}

void telemetry_buffer_reset(void)
{
    telemetry_buffer_init();
}

void telemetry_buffer_push(const char *phase, const motor_pid_telemetry_t *m1, const motor_pid_telemetry_t *m3, float gyro_z, int imu_status)
{
    telemetry_sample_t sample = {0};

    sample.sequence = ++s_sequence;
    sample.timestamp_ms = esp_timer_get_time() / 1000;
    if (phase != NULL) {
        strncpy(sample.phase, phase, TELEMETRY_PHASE_MAX_LEN - 1);
        sample.phase[TELEMETRY_PHASE_MAX_LEN - 1] = '\0';
    }
    if (m1 != NULL) {
        sample.m1 = *m1;
    }
    if (m3 != NULL) {
        sample.m3 = *m3;
    }
    sample.gyro_z = gyro_z;
    sample.imu_status = imu_status;

    portENTER_CRITICAL(&s_lock);
    if (!s_initialized) {
        memset(s_samples, 0, sizeof(s_samples));
        s_initialized = true;
    }

    s_samples[s_head] = sample;
    s_head = (s_head + 1) % TELEMETRY_BUFFER_CAPACITY;
    if (s_count < TELEMETRY_BUFFER_CAPACITY) {
        s_count++;
    }
    portEXIT_CRITICAL(&s_lock);
}

size_t telemetry_buffer_copy_latest(telemetry_sample_t *out_samples, size_t max_samples)
{
    size_t copied = 0;

    if (out_samples == NULL || max_samples == 0) {
        return 0;
    }

    portENTER_CRITICAL(&s_lock);
    if (!s_initialized || s_count == 0) {
        portEXIT_CRITICAL(&s_lock);
        return 0;
    }

    copied = (s_count < max_samples) ? s_count : max_samples;
    size_t start = (s_head + TELEMETRY_BUFFER_CAPACITY - copied) % TELEMETRY_BUFFER_CAPACITY;
    for (size_t i = 0; i < copied; ++i) {
        size_t index = (start + i) % TELEMETRY_BUFFER_CAPACITY;
        out_samples[i] = s_samples[index];
    }
    portEXIT_CRITICAL(&s_lock);

    return copied;
}

size_t telemetry_buffer_count(void)
{
    size_t count = 0;

    portENTER_CRITICAL(&s_lock);
    count = s_count;
    portEXIT_CRITICAL(&s_lock);

    return count;
}
