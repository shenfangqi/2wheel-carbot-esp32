#pragma once

#define CARBOT_DEGREES_TO_RADIANS 0.017453292519943295
#define CARBOT_STANDARD_GRAVITY_MPS2 9.80665

static inline double carbot_dps_to_rad_s(float value)
{
    return value * CARBOT_DEGREES_TO_RADIANS;
}

static inline double carbot_g_to_m_s2(float value)
{
    return value * CARBOT_STANDARD_GRAVITY_MPS2;
}
