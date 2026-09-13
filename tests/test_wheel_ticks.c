#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "control/wheel_ticks_math.h"

int main(void)
{
    assert(wheel_ticks_accumulate(100, 10, 15) == 105);
    assert(wheel_ticks_accumulate(100, 15, 10) == 95);
    assert(wheel_ticks_accumulate(INT64_C(5000000000), -20, 30) == INT64_C(5000000050));
    assert(wheel_ticks_accumulate(0, INT_MAX - 2, INT_MIN + 2) == 5);
    assert(wheel_ticks_accumulate_signed(10, 100, 120, -1) == -10);
    return 0;
}
