#include "network/web_server.h"
#include "control/diff_drive_controller.h"
#include "control/servo_controller.h"
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
        "<style>"
        "body{margin:0;font-family:Arial,sans-serif;background:#f4f6fb;color:#1f2937;}"
        ".page{min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:24px;padding:24px;box-sizing:border-box;}"
        "h1{margin:0;font-size:28px;}"
        ".pad{position:relative;width:260px;height:260px;}"
        ".btn{border:none;background:#2563eb;color:#fff;font-size:18px;font-weight:600;cursor:pointer;box-shadow:0 10px 24px rgba(37,99,235,0.24);}"
        ".btn:active{transform:scale(0.97);}"
        ".circle{position:absolute;width:74px;height:74px;border-radius:50%;}"
        ".up{top:0;left:50%;transform:translateX(-50%);}"
        ".left{top:50%;left:0;transform:translateY(-50%);}"
        ".right{top:50%;right:0;transform:translateY(-50%);}"
        ".down{bottom:0;left:50%;transform:translateX(-50%);}"
        ".center{top:50%;left:50%;transform:translate(-50%,-50%);background:#0f766e;box-shadow:0 10px 24px rgba(15,118,110,0.24);}"
        ".stop{width:74px;height:74px;border-radius:16px;background:#dc2626;box-shadow:0 10px 24px rgba(220,38,38,0.24);}"
        "#status{min-height:24px;margin:0;font-size:18px;font-weight:500;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class='page'>"
        "<h1>Carbot Control</h1>"
        "<div class='pad'>"
        "<button class='btn circle up' onclick=\"sendCmd('forward')\">Up</button>"
        "<button class='btn circle left' onclick=\"sendCmd('left')\">Left</button>"
        "<button class='btn circle center' onclick=\"sendCmd('center')\">Center</button>"
        "<button class='btn circle right' onclick=\"sendCmd('right')\">Right</button>"
        "<button class='btn circle down' onclick=\"sendCmd('backward')\">Down</button>"
        "</div>"
        "<button class='btn stop' onclick=\"sendCmd('stop')\">Stop</button>"
        "<p id='status'>Ready</p>"
        "</div>"
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
                servo_controller_turn_left();
            }
            else if (strcmp(move, "right") == 0) {
                diff_drive_stop();
                servo_controller_turn_right();
            }
            else if (strcmp(move, "center") == 0) {
                diff_drive_stop();
                servo_controller_center();
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
