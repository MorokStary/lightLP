/* control_task.c – зчитування сенсорів, ПІД, PWM */

#include "control_task.h"
#include "sensors.h"
#include "pid_control.h"
#include "pwm_driver.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LOOP_PERIOD_MS  100      /* 10 Гц регулювання */

static const char *TAG = "CONTROL";

static void control_loop(void *arg)
{
    sensor_data_t s;
    float set_lux = storage_get_target_lux();

    for (;;)
    {
        if (sensors_sample(&s) == ESP_OK)
        {
            /* відсутність людей – знижуємо ціль до 20 % */
            float target = s.presence ? set_lux : set_lux * 0.2f;
            float duty = pid_control_update(target, s.lux);

            pwm_driver_set_duty(duty);

            ESP_LOGI(TAG, "Lux %.1f, set %.1f, duty %.2f, T %.1f°C, pres %d",
                     s.lux, target, duty, s.temperature, s.presence);
        }
        vTaskDelay(pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}

esp_err_t control_task_start(void)
{
    /* Параметри ПІД із NVS */
    pid_cfg_t cfg = storage_get_pid_cfg();
    cfg.dt = LOOP_PERIOD_MS / 1000.0f;
    pid_control_init(&cfg);

    xTaskCreatePinnedToCore(control_loop, "ctrl", 4096, NULL, 5, NULL, 1);
    return pwm_driver_init();   /* ініціалізує MCPWM, скважність 0 % */
}
