#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void diff_drive_init(void);
void diff_drive_forward(int speed);
void diff_drive_backward(int speed);
void diff_drive_stop(void);

#ifdef __cplusplus
}
#endif
