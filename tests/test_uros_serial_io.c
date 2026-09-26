#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "network/uros_serial_io.h"

int main(void)
{
    uint8_t error = 99;

    assert(UROS_SERIAL_BAUD_RATE == 921600);
    assert(UROS_SERIAL_FRAMING_ENABLED);
    assert(uros_serial_io_result(0, 16, &error) == 0);
    assert(error == 0); /* timeout is retryable */
    assert(uros_serial_io_result(3, 16, &error) == 3);
    assert(error == 0); /* short read is valid with framing */
    assert(uros_serial_io_result(-1, 16, &error) == 0);
    assert(error == 1);

    uros_serial_state_t state = UROS_SERIAL_CLOSED;
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_OPEN_BEGIN);
    assert(state == UROS_SERIAL_OPENING);
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_OPEN_OK);
    assert(state == UROS_SERIAL_OPEN);
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_IO_ERROR);
    assert(state == UROS_SERIAL_FAILED);
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_CLOSE);
    assert(state == UROS_SERIAL_CLOSED);
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_OPEN_BEGIN);
    state = uros_serial_next_state(state, UROS_SERIAL_EVENT_OPEN_OK);
    assert(state == UROS_SERIAL_OPEN); /* reconnect after failure */

    puts("micro-ROS serial I/O tests passed");
    return 0;
}
