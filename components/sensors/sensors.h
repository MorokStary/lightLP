/* sensors.h – публічний інтерфейс сенсорного модуля */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* Параметри спільної вибірки */
typedef struct
{
    float   lux;          /* освітленість, лк */
    bool    presence;     /* факт наявності руху (PIR) */
    float   temperature;  /* температура, °C */
} sensor_data_t;

/* ініціалізація I²C-шини, GPIO та АЦП */
esp_err_t sensors_init(void);

/* одноразове опитування всіх датчиків (блокує ~30 мс) */
esp_err_t sensors_sample(sensor_data_t *out);

/* окремі швидкі читання у випадку, якщо потрібен лише один параметр */
float  sensors_read_lux(void);          /* останнє виміряне значення, лк   */
bool   sensors_read_presence(void);     /* миттєвий стан GPIO PIR          */
float  sensors_read_temperature(void);  /* останнє виміряне значення, °C   */
