/* wifi_manager.h – автоматичне підключення до Wi-Fi із повторними спробами */

#pragma once
#include <stdbool.h>
#include "esp_err.h"

esp_err_t wifi_manager_init(void);              /* асинхронний запуск STA-режиму */
bool       wifi_manager_is_connected(void);      /* true коли IP отримано       */
const char *wifi_manager_get_ip(void);           /* текстове представлення IP    */
