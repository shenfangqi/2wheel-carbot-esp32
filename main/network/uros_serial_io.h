#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UROS_SERIAL_BAUD_RATE 921600
#define UROS_SERIAL_FRAMING_ENABLED true

typedef enum {
    UROS_SERIAL_CLOSED = 0,
    UROS_SERIAL_OPENING,
    UROS_SERIAL_OPEN,
    UROS_SERIAL_FAILED,
} uros_serial_state_t;

typedef enum {
    UROS_SERIAL_EVENT_OPEN_BEGIN = 0,
    UROS_SERIAL_EVENT_OPEN_OK,
    UROS_SERIAL_EVENT_IO_ERROR,
    UROS_SERIAL_EVENT_CLOSE,
} uros_serial_event_t;

static inline uros_serial_state_t uros_serial_next_state(
    uros_serial_state_t current, uros_serial_event_t event)
{
    (void)current;
    switch (event) {
    case UROS_SERIAL_EVENT_OPEN_BEGIN: return UROS_SERIAL_OPENING;
    case UROS_SERIAL_EVENT_OPEN_OK: return UROS_SERIAL_OPEN;
    case UROS_SERIAL_EVENT_IO_ERROR: return UROS_SERIAL_FAILED;
    case UROS_SERIAL_EVENT_CLOSE:
    default: return UROS_SERIAL_CLOSED;
    }
}

/* UART timeout and short reads are valid XRCE stream behavior. Only a driver
 * error is reported through the custom transport error flag. */
static inline size_t uros_serial_io_result(int io_result, size_t requested,
                                           uint8_t *error)
{
    if (error != NULL) {
        *error = io_result < 0 ? 1U : 0U;
    }
    if (io_result <= 0) {
        return 0;
    }
    return (size_t)io_result > requested ? requested : (size_t)io_result;
}
