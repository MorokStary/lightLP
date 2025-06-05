/* ota_service.h – запуск OTA за командою (наприклад, з MQTT) */

#pragma once
#include "esp_err.h"

/* ініціалізує підтримку OTA-сервісу та реєструє команду */
esp_err_t ota_service_init(void);
