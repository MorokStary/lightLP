// main.c — точка входу кіберфізичної системи керування освітленням

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Публічні заголовки з компонентів
#include "control_task.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "web_server.h"
#include "storage.h"
#include "ota_service.h"

static const char *TAG = "MAIN";

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    else
    {
        ESP_ERROR_CHECK(ret);
    }
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "System booting...");

    // Ініціалізація базових сервісів
    init_nvs();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Зчитування налаштувань з NVS
    ESP_ERROR_CHECK(storage_init());

    // Ініціалізація мережевих інтерфейсів
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(mqtt_client_init());
    ESP_ERROR_CHECK(web_server_start());

    // Ініціалізація сенсорів і PWM
    ESP_ERROR_CHECK(sensors_init());
    ESP_ERROR_CHECK(control_task_start());

    // Ініціалізація OTA сервісу (через MQTT)
    ESP_ERROR_CHECK(ota_service_init());

    ESP_LOGI(TAG, "System started successfully");
}
