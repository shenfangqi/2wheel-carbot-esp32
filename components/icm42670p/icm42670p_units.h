#pragma once

#include <stdint.h>

#define ICM42670P_STANDARD_GRAVITY_M_S2 9.80665f
#define ICM42670P_PI 3.14159265358979323846f

static inline float icm42670p_raw_to_accel_m_s2(int16_t raw, uint16_t fsr_g)
{
    return ((float)raw * (float)fsr_g * ICM42670P_STANDARD_GRAVITY_M_S2) /
           (float)INT16_MAX;
}

static inline float icm42670p_raw_to_gyro_rad_s(int16_t raw, uint16_t fsr_dps)
{
    return ((float)raw * (float)fsr_dps * ICM42670P_PI) /
           (180.0f * (float)INT16_MAX);
}
