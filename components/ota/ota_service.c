/* ota_service.c – запускає OTA після прийому MQTT-команди */

#include "ota_service.h"
#include "mqtt_client.h"
#include "ota_update.h"
#include "esp_log.h"

#define OTA_TOPIC   "BKR/LPNU/Lighting/OTA"

static const char *TAG = "OTA_SVC";

/* Обробник окремо зареєстрованого OTA-потоку */
static esp_err_t handle_ota_command(const char *payload, int len)
{
    char url[256] = {0};
    if (len >= sizeof(url)) return ESP_ERR_INVALID_ARG;
    strncpy(url, payload, len);
    url[len] = '\0';

    ESP_LOGI(TAG, "Received OTA URL: %s", url);
    return ota_update_start(url);
}

static esp_err_t mqtt_cb(esp_mqtt_event_handle_t e)
{
    if (e->event_id == MQTT_EVENT_DATA &&
        strncmp(e->topic, OTA_TOPIC, e->topic_len) == 0)
    {
        return handle_ota_command(e->data, e->data_len);
    }
    return ESP_OK;
}

esp_err_t ota_service_init(void)
{
    esp_mqtt_client_subscribe(mqtt_get_client(), OTA_TOPIC, 1);
    esp_mqtt_client_register_event(mqtt_get_client(), MQTT_EVENT_DATA, mqtt_cb, NULL);
    return ESP_OK;
}
