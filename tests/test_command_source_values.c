#include <assert.h>

#include "control/command_mux.h"

int main(void)
{
    assert(COMMAND_SOURCE_NONE == 0);
    assert(COMMAND_SOURCE_ROS == 2);
    return 0;
}
