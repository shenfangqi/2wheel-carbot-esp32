#include <assert.h>
#include <stdint.h>

#include "ros_interface/ros_time_state.h"

int main(void)
{
    ros_time_sync_state_t state;
    ros_time_sync_state_reset(&state);

    ros_time_sync_state_record_failure(&state);
    assert(!state.synchronized);
    assert(state.current_offset_ns == 0);
    assert(state.target_offset_ns == 0);
    assert(state.last_sync_us == 0);
    assert(state.sync_fail_count == 1);

    ros_time_sync_state_record_success(&state, INT64_C(2500000), INT64_C(1000000));
    assert(state.synchronized);
    assert(state.current_offset_ns == INT64_C(2500000));
    assert(state.target_offset_ns == INT64_C(2500000));
    assert(state.last_sync_us == INT64_C(1000000));

    state.current_offset_ns = INT64_C(2600000);
    ros_time_sync_state_record_success(&state, -INT64_C(1500000), INT64_C(2000000));
    assert(state.current_offset_ns == INT64_C(2600000));
    assert(state.target_offset_ns == -INT64_C(1500000));
    assert(state.last_sync_us == INT64_C(2000000));

    ros_time_sync_state_record_failure(&state);
    assert(state.synchronized);
    assert(state.current_offset_ns == INT64_C(2600000));
    assert(state.target_offset_ns == -INT64_C(1500000));
    assert(state.last_sync_us == INT64_C(2000000));
    assert(state.sync_fail_count == 2);
    return 0;
}
