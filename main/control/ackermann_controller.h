#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ackermann_controller_init(void);
void ackermann_controller_set_cmd(float linear_mps, float angular_rps);
void ackermann_controller_stop(void);

#ifdef __cplusplus
}
#endif
