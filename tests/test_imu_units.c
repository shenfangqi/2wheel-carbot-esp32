#include <assert.h>
#include <math.h>

#include "ros_interface/imu_units.h"

int main(void)
{
    assert(fabs(carbot_dps_to_rad_s(180.0f) - 3.141592653589793) < 1e-9);
    assert(fabs(carbot_g_to_m_s2(1.0f) - 9.80665) < 1e-9);
    return 0;
}
