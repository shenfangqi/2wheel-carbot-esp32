#include <assert.h>
#include <math.h>

#include "icm42670p_units.h"

int main(void)
{
    assert(fabsf(icm42670p_raw_to_accel_m_s2(INT16_MAX, 4) - 39.2266f) < 1e-5f);
    assert(fabsf(icm42670p_raw_to_accel_m_s2(INT16_MAX / 4, 4) - 9.80665f) < 0.001f);
    assert(fabsf(icm42670p_raw_to_gyro_rad_s(INT16_MAX, 2000) -
                 (2000.0f * ICM42670P_PI / 180.0f)) < 1e-5f);
    return 0;
}
