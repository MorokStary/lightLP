/* control_task.h – FreeRTOS-завдання замкненого контуру */

#pragma once
#include "esp_err.h"

/* створює задачу та повертає статус */
esp_err_t control_task_start(void);
