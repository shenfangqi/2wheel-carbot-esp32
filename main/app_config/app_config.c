#include "app_config.h"
#include "config_store.h"
#include <string.h>
#include <stdio.h>

static app_config_t g_config;

void app_config_init(void)
{
    // 默认值
    strcpy(g_config.wifi_ssid, "default_ssid");
    strcpy(g_config.wifi_password, "12345678");
    strcpy(g_config.agent_ip, "192.168.1.100");
    g_config.agent_port = 8888;

    // 覆盖为NVS
    config_store_load(&g_config);

    printf("config loaded\n");
}

app_config_t* app_config_get(void)
{
    return &g_config;
}
