#include "network/web_server.h"
#include "control/diff_drive_controller.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "web_server";
static httpd_handle_t s_server = NULL;

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>Carbot Control</title>"
        "</head>"
        "<body>"
        "<h1>Carbot Control</h1>"
        "<div style='display:flex;flex-direction:column;gap:10px;width:200px;'>"
        "<button onclick=\"sendCmd('forward')\">Forward</button>"
        "<button onclick=\"sendCmd('backward')\">Backward</button>"
        "<button onclick=\"sendCmd('left')\">Left</button>"
        "<button onclick=\"sendCmd('right')\">Right</button>"
        "<button onclick=\"sendCmd('stop')\">Stop</button>"
        "</div>"
        "<p id='status'>Ready</p>"
        "<script>"
        "function sendCmd(cmd){"
        "  fetch('/cmd?move=' + cmd)"
        "    .then(r => r.text())"
        "    .then(t => document.getElementById('status').innerText = t)"
        "    .catch(e => document.getElementById('status').innerText = 'error');"
        "}"
        "</script>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t cmd_get_handler(httpd_req_t *req)
{
    char query[128] = {0};
    char move[32] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "move", move, sizeof(move)) == ESP_OK) {
            ESP_LOGI(TAG, "web cmd: %s", move);

            if (strcmp(move, "forward") == 0) {
                diff_drive_forward(50);
            }
            else if (strcmp(move, "backward") == 0) {
                diff_drive_backward(50);
            }
            else if (strcmp(move, "stop") == 0) {
                diff_drive_stop();
            } 
            else if (strcmp(move, "left") == 0) {
                diff_drive_stop();
            }
            else if (strcmp(move, "right") == 0) {
                diff_drive_stop();
            }
            else {
                ESP_LOGW(TAG, "unknown move: %s", move);
            }

            httpd_resp_set_type(req, "text/plain; charset=utf-8");
            httpd_resp_sendstr(req, move);
            return ESP_OK;
        }
    }

    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "missing move");
    return ESP_OK;
}

void web_server_start(void)
{
    if (s_server != NULL) {
        ESP_LOGI(TAG, "web server already started");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    ESP_LOGI(TAG, "starting web server on port %d", config.server_port);

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to start web server: %s", esp_err_to_name(err));
        s_server = NULL;
        return;
    }

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri = "/cmd",
        .method = HTTP_GET,
        .handler = cmd_get_handler,
        .user_ctx = NULL
    };

    err = httpd_register_uri_handler(s_server, &root_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to register root handler: %s", esp_err_to_name(err));
    }

    err = httpd_register_uri_handler(s_server, &cmd_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to register cmd handler: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "web server started");
}