#include "app_config/usb_config_cli.h"
#include "app_config/app_config.h"
#include "app_config/config_store.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_system.h"

#define CLI_UART_NUM        UART_NUM_0
#define CLI_RX_BUF_SIZE     256
#define CLI_TX_BUF_SIZE     0
#define CLI_LINE_BUF_SIZE   128

static void print_prompt(void)
{
    printf("carbot> ");
    fflush(stdout);
}

static void handle_command(char *line)
{
    app_config_t *cfg = app_config_get();

    if (strcmp(line, "show") == 0) {
        printf("wifi_ssid=%s\n", cfg->wifi_ssid);
        printf("wifi_password=%s\n", cfg->wifi_password);
        printf("agent_ip=%s\n", cfg->agent_ip);
        printf("agent_port=%d\n", cfg->agent_port);
    }
    else if (strncmp(line, "set ", 4) == 0) {
        char key[32] = {0};
        char value[64] = {0};

        if (sscanf(line + 4, "%31s %63s", key, value) == 2) {
            if (strcmp(key, "wifi_ssid") == 0) {
                strncpy(cfg->wifi_ssid, value, sizeof(cfg->wifi_ssid) - 1);
                printf("OK\n");
            } else if (strcmp(key, "wifi_password") == 0) {
                strncpy(cfg->wifi_password, value, sizeof(cfg->wifi_password) - 1);
                printf("OK\n");
            } else if (strcmp(key, "agent_ip") == 0) {
                strncpy(cfg->agent_ip, value, sizeof(cfg->agent_ip) - 1);
                printf("OK\n");
            } else if (strcmp(key, "agent_port") == 0) {
                cfg->agent_port = atoi(value);
                printf("OK\n");
            } else {
                printf("ERR unknown key\n");
            }
        } else {
            printf("ERR usage: set <key> <value>\n");
        }
    }
    else if (strcmp(line, "save") == 0) {
        config_store_save(cfg);
        printf("saved\n");
    }
    else if (strcmp(line, "reboot") == 0) {
        printf("rebooting...\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }
    else if (strlen(line) == 0) {
        // 空行不处理
    }
    else {
        printf("ERR unknown command\n");
    }
}

static void cli_task(void *arg)
{
    uint8_t ch;
    char line[CLI_LINE_BUF_SIZE];
    int idx = 0;

    printf("uart cli ready\n");
    print_prompt();

    while (1) {
        int len = uart_read_bytes(CLI_UART_NUM, &ch, 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            if (ch == '\r' || ch == '\n') {
                printf("\n");
                line[idx] = '\0';
                handle_command(line);
                idx = 0;
                memset(line, 0, sizeof(line));
                print_prompt();
            } else if (ch == 0x08 || ch == 0x7F) {
                if (idx > 0) {
                    idx--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else {
                if (idx < CLI_LINE_BUF_SIZE - 1) {
                    line[idx++] = (char)ch;
                    printf("%c", ch);
                    fflush(stdout);
                }
            }
        }
    }
}

void usb_cli_start(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION_MAJOR >= 5
        .source_clk = UART_SCLK_DEFAULT,
#endif
    };

    uart_driver_install(CLI_UART_NUM, CLI_RX_BUF_SIZE, CLI_TX_BUF_SIZE, 0, NULL, 0);
    uart_param_config(CLI_UART_NUM, &uart_config);

    xTaskCreate(cli_task, "cli_task", 4096, NULL, 5, NULL);
}