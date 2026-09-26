#include "config_store.h"
#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "carbot"

void config_store_load(app_config_t *cfg)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }
    int32_t servo_center_offset_deg;

    if (nvs_get_i32(handle, "servo_ofs", &servo_center_offset_deg) == ESP_OK) {
        cfg->servo_center_offset_deg = (int)servo_center_offset_deg;
    }

    nvs_close(handle);
}

void config_store_save(app_config_t *cfg)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    nvs_set_i32(handle, "servo_ofs", cfg->servo_center_offset_deg);

    nvs_commit(handle);
    nvs_close(handle);

}
