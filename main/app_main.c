#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_config/app_config.h"
#include "app_config/usb_config_cli.h"
#include "network/wifi_manager.h"
#include "control/diff_drive_controller.h"

void app_main(void)
{
    nvs_flash_init();

    printf("=== CARBOT START ===\n");

    app_config_init();
    printf("config init ok\n");

    usb_cli_start();
    printf("cli start ok\n");

    diff_drive_init();
    printf("diff drive init ok\n");

    wifi_manager_init();
    printf("wifi manager init ok\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}