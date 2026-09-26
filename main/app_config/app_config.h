#pragma once

typedef struct {
    int servo_center_offset_deg;
} app_config_t;

void app_config_init(void);
app_config_t* app_config_get(void);
void app_config_save(void);
