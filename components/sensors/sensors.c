/* sensors.c – реалізація драйверів BH1750, PIR та NTC */

#include "sensors.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_timer.h"
#include "esp_log.h"

#define I2C_PORT            I2C_NUM_0
#define I2C_SDA_PIN         17
#define I2C_SCL_PIN         18
#define BH1750_ADDR         0x23
#define BH1750_ON_CMD       0x01
#define BH1750_CONT_HI_RES  0x10

#define PIR_GPIO            23

#define NTC_ADC_CHANNEL     ADC1_CHANNEL_6   /* GPIO34 */
#define ADC_ATTEN           ADC_ATTEN_DB_11
#define ADC_WIDTH_          ADC_WIDTH_BIT_12

/* NTC характеристика: R25 = 100 кОм, B = 3950 K */
#define NTC_R_SERIES        100000.0f   /* резистор подільника 100 кОм */
#define NTC_R25             100000.0f
#define NTC_BETA            3950.0f
#define KELVIN_25           298.15f

static const char *TAG = "SENSORS";
static esp_adc_cal_characteristics_t adc_chars;
static float last_lux = 0.0f;
static float last_temp = 0.0f;

static esp_err_t bh1750_write(uint8_t cmd)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (BH1750_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, cmd, true);
    i2c_master_stop(h);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(h);
    return err;
}

static esp_err_t bh1750_read_raw(uint16_t *raw)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (BH1750_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(h, ((uint8_t *)raw) + 1, I2C_MASTER_ACK);
    i2c_master_read_byte(h, (uint8_t *)raw,       I2C_MASTER_NACK);
    i2c_master_stop(h);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(h);
    /* BH1750 віддає big-endian, ми вже розмістили байти у вірному порядку */
    return err;
}

/* конвертація АЦП -> °C за спрощеною Beta-моделлю */
static float ntc_to_celsius(uint32_t adc_raw)
{
    float v_meas = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars) / 1000.0f;          /* у вольтах */
    float v_ref  = 3.3f;
    float r_ntc  = (v_meas * NTC_R_SERIES) / (v_ref - v_meas);                         /* закон подільника */
    float temp_k = 1.0f / ( (1.0f / KELVIN_25) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R25) );
    return temp_k - 273.15f;
}

esp_err_t sensors_init(void)
{
    /* I²C */
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    /* BH1750: увімкнення та перехід у безперервний режим */
    ESP_ERROR_CHECK(bh1750_write(BH1750_ON_CMD));
    ESP_ERROR_CHECK(bh1750_write(BH1750_CONT_HI_RES));

    /* PIR */
    gpio_config_t pir = {
        .pin_bit_mask = 1ULL << PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&pir));

    /* ADC */
    adc1_config_width(ADC_WIDTH_);
    adc1_config_channel_atten(NTC_ADC_CHANNEL, ADC_ATTEN);
    esp_adc_cal_value_t t = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_, 0, &adc_chars);
    ESP_LOGI(TAG, "ADC calibration type %d", t);

    /* перше вимірювання */
    sensor_data_t tmp;
    return sensors_sample(&tmp);
}

esp_err_t sensors_sample(sensor_data_t *out)
{
    /* BH1750: дані готові приблизно за 120 мс після старту режиму;
       але ми працюємо у безперервному – затримка не потрібна */
    uint16_t raw = 0;
    ESP_RETURN_ON_ERROR(bh1750_read_raw(&raw), TAG, "BH1750 read");
    last_lux = (float)raw / 1.2f; /* за даташитом, коеф. 1.2 */

    /* PIR */
    out->presence = gpio_get_level(PIR_GPIO);

    /* ADC */
    uint32_t adc_raw = adc1_get_raw(NTC_ADC_CHANNEL);
    last_temp = ntc_to_celsius(adc_raw);

    /* підготовка пакета */
    out->lux        = last_lux;
    out->temperature = last_temp;

    return ESP_OK;
}

float sensors_read_lux(void)          { return last_lux; }
bool  sensors_read_presence(void)     { return gpio_get_level(PIR_GPIO); }
float sensors_read_temperature(void)  { return last_temp; }
