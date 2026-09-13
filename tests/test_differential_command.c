#include <assert.h>

#include "control/differential_controller.h"

int main(void)
{
    assert(differential_command_is_stopped(0.0f, 0.0f));
    assert(differential_command_is_stopped(0.00001f, -0.00001f));
    assert(!differential_command_is_stopped(0.1f, 0.0f));
    assert(!differential_command_is_stopped(0.0f, 0.1f));
    assert(differential_command_snapshot_is_current(7U, 7U));
    assert(!differential_command_snapshot_is_current(7U, 8U));
    assert(!differential_command_snapshot_is_current(UINT32_MAX, 0U));
    assert(differential_heading_hold_should_activate(true, true, true, true));
    assert(!differential_heading_hold_should_activate(false, true, true, true));
    assert(!differential_heading_hold_should_activate(true, false, true, true));
    assert(!differential_heading_hold_should_activate(true, true, false, true));
    assert(!differential_heading_hold_should_activate(true, true, true, false));
    return 0;
}
