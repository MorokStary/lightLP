/* mqtt_client.c – використано esp-mqtt-component */

#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "wifi_manager.h"
#include "cJSON.h"
#include "storage.h"
#include "control_task.h"

#define MQTT_URI        "broker.hivemq.com"
#define TOPIC_STATUS    "BKR/LPNU/Lighting/Status"
#define TOPIC_CMD       "BKR/LPNU/Lighting/Cmd"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client;

static esp_err_t cmd_handler(const char *data, int len)
{
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) return ESP_FAIL;

    /* очікуємо {"target_lux":450.0} */
    cJSON *t = cJSON_GetObjectItem(root, "target_lux");
    if (cJSON_IsNumber(t))
    {
        storage_set_target_lux((float)t->valuedouble);
    }
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t ev)
{
    switch (ev->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "connected");
            esp_mqtt_client_subscribe(client, TOPIC_CMD, 1);
            break;

        case MQTT_EVENT_DATA:
            if (strncmp(ev->topic, TOPIC_CMD, ev->topic_len) == 0)
            {
                cmd_handler(ev->data, ev->data_len);
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

esp_err_t mqtt_client_init(void)
{
    esp_mqtt_client_config_t cfg = {
        .uri            = MQTT_URI,
        .network.timeout_ms = 5000,
        .cert_pem       = NULL,             /* hiveMQ – без TLS за замовч. */
        .task_stack      = 4096
    };
    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);
    return ESP_OK;
}

void mqtt_client_publish(const sensor_data_t *d)
{
    if (!wifi_manager_is_connected()) return;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "lux", d->lux);
    cJSON_AddNumberToObject(root, "temp", d->temperature);
    cJSON_AddBoolToObject  (root, "presence", d->presence);
    char *msg = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(client, TOPIC_STATUS, msg, 0, 1, 0);
    cJSON_FreeString(msg);
    cJSON_Delete(root);
}
