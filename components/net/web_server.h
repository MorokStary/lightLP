/* web_server.h – HTTPS-інтерфейс налаштувань (JWT) */

#pragma once
#include "esp_err.h"

/* запуск HTTPS-серверу, блокуючий call повертає одразу після ініціалізації */
esp_err_t web_server_start(void);

/* зупинка (необов’язково викликати під час вимкнення MCU) */
void      web_server_stop(void);
