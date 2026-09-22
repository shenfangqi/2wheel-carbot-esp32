#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool synchronized;
    int64_t current_offset_ns;
    int64_t target_offset_ns;
    int64_t last_sync_us;
    uint32_t sync_fail_count;
} ros_time_sync_state_t;

static inline void ros_time_sync_state_reset(ros_time_sync_state_t *state)
{
    *state = (ros_time_sync_state_t){0};
}

static inline void ros_time_sync_state_record_failure(ros_time_sync_state_t *state)
{
    state->sync_fail_count++;
}

static inline void ros_time_sync_state_record_success(ros_time_sync_state_t *state,
                                                      int64_t offset_ns,
                                                      int64_t sync_time_us)
{
    state->target_offset_ns = offset_ns;
    if (!state->synchronized) {
        state->current_offset_ns = offset_ns;
    }
    state->last_sync_us = sync_time_us;
    state->synchronized = true;
}
