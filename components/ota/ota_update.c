/* ota_update.c – OTA через esp_https_ota */

#include "ota_update.h"
#include "esp_https_ota.h"
#include "esp_log.h"

static const char *TAG = "OTA";

esp_err_t ota_update_start(const char *url)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
        .cert_pem = NULL   /* для публічних HTTPS (можна додати root.pem) */
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &cfg
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Firmware updated, restarting...");
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }

    return ret;
}
