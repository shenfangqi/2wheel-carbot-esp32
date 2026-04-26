#include "network/web_server.h"
#include "app_config/app_config.h"
#include "control/diff_drive_controller.h"
#include "control/servo_controller.h"
#include "utils/telemetry_buffer.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
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
        ".page{min-height:10vh;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:24px;padding:24px;box-sizing:border-box;}"
        "h1{margin:0;font-size:28px;}"
        ".pad{position:relative;width:260px;height:260px;}"
        ".btn{border:none;background:#2563eb;color:#fff;font-size:18px;font-weight:600;cursor:pointer;box-shadow:0 10px 24px rgba(37,99,235,0.24);touch-action:none;-webkit-user-select:none;user-select:none;}"
        ".btn.active{filter:brightness(1.12);box-shadow:0 0 0 4px rgba(255,255,255,0.9),0 0 0 8px rgba(37,99,235,0.28),0 14px 30px rgba(37,99,235,0.34);}"
        ".circle{position:absolute;width:74px;height:74px;border-radius:16px;}"
        ".smallbtn{padding:10px 14px;border-radius:12px;font-size:16px;box-shadow:none;}"
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
        "<h1>Carbot Control</h1><br><br>"
        "<div class='pad'>"
        "<button id='btn-forward' class='btn circle up' onpointerdown=\"return startDrive('forward', event)\" onpointerup=\"return stopDrive(event)\" onpointercancel=\"return stopDrive(event)\" onpointerleave=\"return stopDrive(event)\">&#9650;</button>"
        "<button id='btn-left' class='btn circle left' onpointerdown=\"return startSteer('left', event)\" onpointerup=\"return stopSteer(event)\" onpointercancel=\"return stopSteer(event)\" onpointerleave=\"return stopSteer(event)\">&#9664;</button>"
        "<button id='btn-center' class='btn circle center' onclick=\"return clickCenter(event)\">&#9679;</button>"
        "<button id='btn-right' class='btn circle right' onpointerdown=\"return startSteer('right', event)\" onpointerup=\"return stopSteer(event)\" onpointercancel=\"return stopSteer(event)\" onpointerleave=\"return stopSteer(event)\">&#9654;</button>"
        "<button id='btn-backward' class='btn circle down' onpointerdown=\"return startDrive('backward', event)\" onpointerup=\"return stopDrive(event)\" onpointercancel=\"return stopDrive(event)\" onpointerleave=\"return stopDrive(event)\">&#9660;</button>"
        "</div><br><br>"
        "<button id='btn-stop' class='btn stop' onclick=\"return clickStop(event)\">&#9632;</button>"
        "<br><br><div style='display:flex;gap:12px;flex-wrap:wrap;justify-content:center'>"
        "<button class='btn' style='padding:12px 16px;border-radius:12px;background:#111827;box-shadow:none' onclick=\"return openTelemetry(event)\">Telemetry</button>"
        "<button class='btn' style='padding:12px 16px;border-radius:12px;background:#6b7280;box-shadow:none' onclick=\"return resetTelemetry(event)\">Reset Telemetry</button>"
        "</div>"
        "<div style='display:flex;align-items:center;gap:12px;flex-wrap:wrap;justify-content:center'>"
        "<button class='btn smallbtn' style='background:#7c3aed' onclick=\"return adjustCenterOffset(-1,event)\">Off -</button>"
        "<span id='offset'>Servo Off: --</span>"
        "<button class='btn smallbtn' style='background:#7c3aed' onclick=\"return adjustCenterOffset(1,event)\">Off +</button>"
        "</div>"
        "<br><br><p id='status'>Ready</p>"
        "</div>"
        "<script>"
        "let activeSteer=null;"
        "let activeSteerPointerId=null;"
        "let activeDrive=null;"
        "let activeDrivePointerId=null;"
        "let centerFlashTimer=null;"
        "let stopFlashTimer=null;"
        "function renderStatus(){"
        "  const parts=[];"
        "  if(activeDrive){parts.push(activeDrive);}"
        "  if(activeSteer){parts.push(activeSteer);}"
        "  document.getElementById('status').innerText = parts.length ? parts.join(' + ') : 'Ready';"
        "}"
        "function setButtonActive(cmd,active){"
        "  const el=document.getElementById('btn-' + cmd);"
        "  if(!el){return;}"
        "  if(active){el.classList.add('active');}"
        "  else{el.classList.remove('active');}"
        "}"
        "function flashCenterButton(){"
        "  const el=document.getElementById('btn-center');"
        "  if(!el){return;}"
        "  el.classList.add('active');"
        "  if(centerFlashTimer){clearTimeout(centerFlashTimer);}"
        "  centerFlashTimer=setTimeout(()=>{el.classList.remove('active');centerFlashTimer=null;},180);"
        "}"
        "function flashStopButton(){"
        "  const el=document.getElementById('btn-stop');"
        "  if(!el){return;}"
        "  el.classList.add('active');"
        "  if(stopFlashTimer){clearTimeout(stopFlashTimer);}"
        "  stopFlashTimer=setTimeout(()=>{el.classList.remove('active');stopFlashTimer=null;},180);"
        "}"
        "function sendCmd(cmd){"
        "  fetch('/cmd?move=' + cmd)"
        "    .then(r => r.text())"
        "    .then(() => renderStatus())"
        "    .catch(e => document.getElementById('status').innerText = 'error');"
        "}"
        "function refreshCenterOffset(){"
        "  fetch('/servo/offset')"
        "    .then(r => r.text())"
        "    .then(text => {document.getElementById('offset').innerText = 'Center Offset: ' + text + ' deg';})"
        "    .catch(() => {document.getElementById('offset').innerText = 'Center Offset: error';});"
        "}"
        "function startSteer(cmd,event){"
        "  if(activeSteerPointerId!==null && event && activeSteerPointerId!==event.pointerId){return false;}"
        "  if(activeSteer && activeSteer!==cmd){setButtonActive(activeSteer,false);}"
        "  activeSteer=cmd;"
        "  activeSteerPointerId=event?event.pointerId:null;"
        "  setButtonActive(cmd,true);"
        "  if(event){event.preventDefault();}"
        "  sendCmd(cmd);"
        "  return false;"
        "}"
        "function stopSteer(event){"
        "  if(event){event.preventDefault();}"
        "  if(activeSteerPointerId!==null && event && activeSteerPointerId!==event.pointerId){return false;}"
        "  if(!activeSteer){return false;}"
        "  setButtonActive(activeSteer,false);"
        "  activeSteer=null;"
        "  activeSteerPointerId=null;"
        "  flashCenterButton();"
        "  sendCmd('center');"
        "  return false;"
        "}"
        "function startDrive(cmd,event){"
        "  if(activeDrivePointerId!==null && event && activeDrivePointerId!==event.pointerId){return false;}"
        "  if(activeDrive && activeDrive!==cmd){setButtonActive(activeDrive,false);}"
        "  activeDrive=cmd;"
        "  activeDrivePointerId=event?event.pointerId:null;"
        "  setButtonActive(cmd,true);"
        "  if(event){event.preventDefault();}"
        "  sendCmd(cmd);"
        "  return false;"
        "}"
        "function stopDrive(event){"
        "  if(event){event.preventDefault();}"
        "  if(activeDrivePointerId!==null && event && activeDrivePointerId!==event.pointerId){return false;}"
        "  if(!activeDrive){return false;}"
        "  setButtonActive(activeDrive,false);"
        "  activeDrive=null;"
        "  activeDrivePointerId=null;"
        "  sendCmd('stop');"
        "  return false;"
        "}"
        "function clickCenter(event){"
        "  if(event){event.preventDefault();}"
        "  activeSteer=null;"
        "  activeSteerPointerId=null;"
        "  setButtonActive('left',false);"
        "  setButtonActive('right',false);"
        "  flashCenterButton();"
        "  sendCmd('center');"
        "  return false;"
        "}"
        "function clickStop(event){"
        "  if(event){event.preventDefault();}"
        "  activeDrive=null;"
        "  activeDrivePointerId=null;"
        "  setButtonActive('forward',false);"
        "  setButtonActive('backward',false);"
        "  flashStopButton();"
        "  sendCmd('stop');"
        "  return false;"
        "}"
        "function openTelemetry(event){"
        "  if(event){event.preventDefault();}"
        "  window.open('/telemetry','_blank');"
        "  return false;"
        "}"
        "function resetTelemetry(event){"
        "  if(event){event.preventDefault();}"
        "  fetch('/telemetry/reset')"
        "    .then(r => r.text())"
        "    .then(text => {document.getElementById('status').innerText = text;})"
        "    .catch(() => {document.getElementById('status').innerText = 'telemetry reset error';});"
        "  return false;"
        "}"
        "function adjustCenterOffset(delta,event){"
        "  if(event){event.preventDefault();}"
        "  const move = delta > 0 ? 'center_offset_inc' : 'center_offset_dec';"
        "  fetch('/cmd?move=' + move)"
        "    .then(r => r.text())"
        "    .then(text => {document.getElementById('status').innerText = text; refreshCenterOffset();})"
        "    .catch(() => {document.getElementById('status').innerText = 'center offset error';});"
        "  return false;"
        "}"
        "refreshCenterOffset();"
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
                servo_controller_turn_left();
            }
            else if (strcmp(move, "right") == 0) {
                servo_controller_turn_right();
            }
            else if (strcmp(move, "center") == 0) {
                servo_controller_center();
            }
            else if (strcmp(move, "center_offset_inc") == 0) {
                app_config_t *config = app_config_get();
                int offset = servo_controller_adjust_center_offset(1);
                config->servo_center_offset_deg = offset;
                app_config_save();
                servo_controller_center();
                httpd_resp_set_type(req, "text/plain; charset=utf-8");
                snprintf(move, sizeof(move), "center offset=%d", offset);
                httpd_resp_sendstr(req, move);
                return ESP_OK;
            }
            else if (strcmp(move, "center_offset_dec") == 0) {
                app_config_t *config = app_config_get();
                int offset = servo_controller_adjust_center_offset(-1);
                config->servo_center_offset_deg = offset;
                app_config_save();
                servo_controller_center();
                httpd_resp_set_type(req, "text/plain; charset=utf-8");
                snprintf(move, sizeof(move), "center offset=%d", offset);
                httpd_resp_sendstr(req, move);
                return ESP_OK;
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

static esp_err_t servo_offset_get_handler(httpd_req_t *req)
{
    char response[32] = {0};

    snprintf(response, sizeof(response), "%d", servo_controller_get_center_offset());
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, response);
    return ESP_OK;
}

static esp_err_t telemetry_get_handler(httpd_req_t *req)
{
    size_t sample_capacity = telemetry_buffer_count();
    telemetry_sample_t *samples = NULL;
    size_t count = 0;
    char line[256] = {0};

    if (sample_capacity == 0) {
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        httpd_resp_sendstr(req, "seq,timestamp_ms,phase,m1_target_rpm,m1_actual_rpm,m1_pwm,m3_target_rpm,m3_actual_rpm,m3_pwm,gyro_z,imu_status\n");
        return ESP_OK;
    }

    samples = calloc(sample_capacity, sizeof(telemetry_sample_t));
    if (samples == NULL) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        httpd_resp_sendstr(req, "telemetry allocation failed");
        return ESP_ERR_NO_MEM;
    }

    count = telemetry_buffer_copy_latest(samples, sample_capacity);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr_chunk(req, "seq,timestamp_ms,phase,m1_target_rpm,m1_actual_rpm,m1_pwm,m3_target_rpm,m3_actual_rpm,m3_pwm,gyro_z,imu_status\n");

    for (size_t i = 0; i < count; ++i) {
        int written = snprintf(
            line,
            sizeof(line),
            "%" PRIu32 ",%" PRId64 ",%s,%.2f,%.2f,%d,%.2f,%.2f,%d,%.6f,%d\n",
            samples[i].sequence,
            samples[i].timestamp_ms,
            samples[i].phase,
            samples[i].m1.target_rpm,
            samples[i].m1.actual_rpm,
            samples[i].m1.pwm_output,
            samples[i].m3.target_rpm,
            samples[i].m3.actual_rpm,
            samples[i].m3.pwm_output,
            samples[i].gyro_z,
            samples[i].imu_status);

        if (written < 0) {
            free(samples);
            httpd_resp_sendstr_chunk(req, NULL);
            return ESP_FAIL;
        }

        esp_err_t err = httpd_resp_send_chunk(req, line, (ssize_t)written);
        if (err != ESP_OK) {
            free(samples);
            httpd_resp_sendstr_chunk(req, NULL);
            return err;
        }
    }

    free(samples);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t telemetry_reset_handler(httpd_req_t *req)
{
    telemetry_buffer_reset();
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "telemetry reset");
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

    httpd_uri_t telemetry_uri = {
        .uri = "/telemetry",
        .method = HTTP_GET,
        .handler = telemetry_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t telemetry_reset_uri = {
        .uri = "/telemetry/reset",
        .method = HTTP_GET,
        .handler = telemetry_reset_handler,
        .user_ctx = NULL
    };

    httpd_uri_t servo_offset_uri = {
        .uri = "/servo/offset",
        .method = HTTP_GET,
        .handler = servo_offset_get_handler,
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

    err = httpd_register_uri_handler(s_server, &telemetry_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to register telemetry handler: %s", esp_err_to_name(err));
    }

    err = httpd_register_uri_handler(s_server, &telemetry_reset_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to register telemetry reset handler: %s", esp_err_to_name(err));
    }

    err = httpd_register_uri_handler(s_server, &servo_offset_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to register servo offset handler: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "web server started");
}
