#include "config_store.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdio.h>

#define NVS_NAMESPACE "carbot"

void config_store_load(app_config_t *cfg)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        printf("nvs load failed, using default\n");
        return;
    }

    size_t len;

    len = sizeof(cfg->wifi_ssid);
    nvs_get_str(handle, "ssid", cfg->wifi_ssid, &len);

    len = sizeof(cfg->wifi_password);
    nvs_get_str(handle, "pwd", cfg->wifi_password, &len);

    len = sizeof(cfg->agent_ip);
    nvs_get_str(handle, "ip", cfg->agent_ip, &len);

    int32_t port;

    if (nvs_get_i32(handle, "port", &port) == ESP_OK) {
        cfg->agent_port = (int)port;
    }

    nvs_close(handle);
    printf("config loaded from nvs\n");
}

void config_store_save(app_config_t *cfg)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        printf("nvs open failed\n");
        return;
    }

    nvs_set_str(handle, "ssid", cfg->wifi_ssid);
    nvs_set_str(handle, "pwd", cfg->wifi_password);
    nvs_set_str(handle, "ip", cfg->agent_ip);
    nvs_set_i32(handle, "port", cfg->agent_port);

    nvs_commit(handle);
    nvs_close(handle);

    printf("config saved to nvs\n");
}