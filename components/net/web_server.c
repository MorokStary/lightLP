/* web_server.c – мінімальний esp_https_server з JWT-авторизацією */

#include "web_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "jwt.h"             /* маленька односторінкова б-ка для HS256 */
#include "storage.h"
#include "cJSON.h"

extern const char server_cert_pem_start[] asm("_binary_cert_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_cert_pem_end");
extern const char server_key_pem_start[]  asm("_binary_key_pem_start");
extern const char server_key_pem_end[]    asm("_binary_key_pem_end");

static const char *TAG = "HTTPD";
static httpd_handle_t srv  = NULL;
static const char *JWT_SECRET = CONFIG_HTTP_JWT_SECRET;

static esp_err_t auth_middleware(httpd_req_t *req)
{
    const char *hdr = httpd_req_get_hdr_value_ptr(req, "Authorization");
    if (!hdr || strncmp(hdr, "Bearer ", 7) != 0) return ESP_FAIL;

    const char *token = hdr + 7;
    return jwt_validate_hs256(token, JWT_SECRET) ? ESP_OK : ESP_FAIL;
}

/* GET /config – повертає json параметрів */
static esp_err_t get_cfg_handler(httpd_req_t *req)
{
    if (auth_middleware(req) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "unauth");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "target_lux", storage_get_target_lux());
    char *buf = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    cJSON_FreeString(buf);
    cJSON_Delete(root);
    return ESP_OK;
}

/* POST /config – {"target_lux":500} */
static esp_err_t set_cfg_handler(httpd_req_t *req)
{
    if (auth_middleware(req) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "unauth");

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf));
    if (len <= 0) return ESP_FAIL;

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return ESP_FAIL;
    cJSON *v = cJSON_GetObjectItem(root, "target_lux");
    if (cJSON_IsNumber(v))
        storage_set_target_lux((float)v->valuedouble);
    cJSON_Delete(root);
    httpd_resp_send(req, "ok", 2);
    return ESP_OK;
}

esp_err_t web_server_start(void)
{
    httpd_ssl_config_t cfg = HTTPD_SSL_CONFIG_DEFAULT();
    cfg.servercert = (const unsigned char *)server_cert_pem_start;
    cfg.servercert_len = server_cert_pem_end - server_cert_pem_start;
    cfg.prvtkey_pem = (const unsigned char *)server_key_pem_start;
    cfg.prvtkey_len = server_key_pem_end - server_key_pem_start;

    httpd_config_t *hc = &cfg.httpd;
    hc->uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_ssl_start(&srv, &cfg) != ESP_OK) return ESP_FAIL;

    static const httpd_uri_t get_cfg = {
        .uri      = "/config",
        .method   = HTTP_GET,
        .handler  = get_cfg_handler
    };
    static const httpd_uri_t set_cfg = {
        .uri      = "/config",
        .method   = HTTP_POST,
        .handler  = set_cfg_handler
    };
    httpd_register_uri_handler(srv, &get_cfg);
    httpd_register_uri_handler(srv, &set_cfg);
    ESP_LOGI(TAG, "HTTPS server started");
    return ESP_OK;
}

void web_server_stop(void)
{
    if (srv) httpd_ssl_stop(srv);
}
