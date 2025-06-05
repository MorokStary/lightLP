/* pwm_driver.h – керування MCPWM-каналом (GPIO4) */

#pragma once
#include "esp_err.h"

/* ініціалізація MCPWM, частота ~1 кГц, початкова скважність 0 % */
esp_err_t pwm_driver_init(void);

/* встановити скважність (0.0 … 1.0) */
esp_err_t pwm_driver_set_duty(float duty);
