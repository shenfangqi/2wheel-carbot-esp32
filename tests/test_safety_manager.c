#include <assert.h>
#include <math.h>

#include "control/safety_manager.h"

static int near(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

int main(void)
{
    safety_limiter_t limiter = {0};
    float linear;
    float angular;

    assert(safety_manager_validate(0.5f, -3.5f));
    assert(!safety_manager_validate(0.51f, 0.0f));
    assert(!safety_manager_validate(0.0f, 3.51f));
    assert(!safety_manager_validate(0.0f, INFINITY));
    assert(!safety_manager_validate(NAN, 0.0f));

    safety_manager_limit_acceleration(&limiter, 0.0f, 0.0f, 1000000, &linear, &angular);
    safety_manager_limit_acceleration(&limiter, 0.5f, 2.0f, 1100000, &linear, &angular);
    assert(near(linear, 0.05f));
    assert(near(angular, 0.25f));
    safety_manager_reset_limiter(&limiter);
    assert(!limiter.initialized);
    return 0;
}
