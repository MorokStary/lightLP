/* ota_update.h – приймання прошивки через HTTPS, перевірка SHA-256 */

#pragma once
#include "esp_err.h"

/* запуск OTA-оновлення (асинхронно), URL має вказувати на .bin файл */
esp_err_t ota_update_start(const char *url);
