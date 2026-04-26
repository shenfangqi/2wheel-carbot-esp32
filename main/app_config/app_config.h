#pragma once

typedef struct {
    char wifi_ssid[32];
    char wifi_password[64];
    char agent_ip[32];
    int agent_port;
    int servo_center_offset_deg;
} app_config_t;

void app_config_init(void);
app_config_t* app_config_get(void);
void app_config_save(void);
