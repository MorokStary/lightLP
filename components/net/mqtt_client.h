/* mqtt_client.h – MQTT: телеметрія + прийом команд */

#pragma once
#include "esp_err.h"

esp_err_t mqtt_client_init(void);                                            /* неблокуючий запуск */
esp_err_t mqtt_client_publish(const char *topic, const char *payload, int qos);

typedef void (*mqtt_cmd_callback_t)(const char *topic, const char *payload);
void mqtt_client_register_cmd_callback(mqtt_cmd_callback_t cb);
