/* storage.c – реалізація на основі NVS */

#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NS_NAME         "config"
#define KEY_LUX         "target_lux"
#define KEY_KP          "pid_kp"
#define KEY_KI          "pid_ki"
#define KEY_KD          "pid_kd"

static const char *TAG = "STORAGE";

static float lux = 400.0f;   /* дефолтна ціль */
static pid_cfg_t cfg = {
    .kp = 0.5f,
    .ki = 0.1f,
    .kd = 0.0f,
    .i_min = -1.0f,
    .i_max =  1.0f,
    .out_min = 0.0f,
    .out_max = 1.0f,
    .dt = 0.1f  /* буде оновлено */
};

esp_err_t storage_init(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS_NAME, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    nvs_get_float(h, KEY_LUX, &lux);
    nvs_get_float(h, KEY_KP,  &cfg.kp);
    nvs_get_float(h, KEY_KI,  &cfg.ki);
    nvs_get_float(h, KEY_KD,  &cfg.kd);
    nvs_close(h);

    ESP_LOGI(TAG, "Config: lux=%.1f kp=%.2f ki=%.2f kd=%.2f", lux, cfg.kp, cfg.ki, cfg.kd);
    return ESP_OK;
}

float storage_get_target_lux(void) { return lux; }

void storage_set_target_lux(float l)
{
    lux = l;
    nvs_handle_t h;
    if (nvs_open(NS_NAME, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_float(h, KEY_LUX, lux);
        nvs_commit(h);
        nvs_close(h);
    }
}

pid_cfg_t storage_get_pid_cfg(void) { return cfg; }

void storage_set_pid_cfg(const pid_cfg_t *in)
{
    cfg.kp = in->kp;
    cfg.ki = in->ki;
    cfg.kd = in->kd;

    nvs_handle_t h;
    if (nvs_open(NS_NAME, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_float(h, KEY_KP, cfg.kp);
        nvs_set_float(h, KEY_KI, cfg.ki);
        nvs_set_float(h, KEY_KD, cfg.kd);
        nvs_commit(h);
        nvs_close(h);
    }
}
