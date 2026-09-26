#include "app_config.h"
#include "config_store.h"
#include "control/servo_controller.h"

static app_config_t g_config;

void app_config_init(void)
{
    g_config.servo_center_offset_deg = SERVO_STEER_DEFAULT_CENTER_OFFSET_DEG;
    config_store_load(&g_config);
}

app_config_t* app_config_get(void)
{
    return &g_config;
}

void app_config_save(void)
{
    config_store_save(&g_config);
}
