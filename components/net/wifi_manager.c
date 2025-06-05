/* wifi_manager.c */

#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"

#define WIFI_SSID       "Gola"
#define WIFI_PASS       "Gola1234"
#define MAX_RETRY       10

static const char *TAG = "WIFI";
static int retry_cnt = 0;
static bool connected = false;
static char ip_str[16] = {0};

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        connected = false;
        if (retry_cnt < MAX_RETRY)
        {
            esp_wifi_connect();
            retry_cnt++;
            ESP_LOGW(TAG, "retry to connect AP");
        }
        else
            ESP_LOGE(TAG, "connect failed");
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof ip_str);
        retry_cnt = 0;
        connected = true;
        ESP_LOGI(TAG, "got IP %s", ip_str);
    }
}

esp_err_t wifi_manager_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst_any;
    esp_event_handler_instance_t inst_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, &inst_any);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, &inst_got_ip);

    wifi_config_t sta = {0};
    strcpy((char *)sta.sta.ssid, WIFI_SSID);
    strcpy((char *)sta.sta.password, WIFI_PASS);
    sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi init finished");
    return ESP_OK;
}

bool wifi_manager_is_connected(void) { return connected; }
const char *wifi_manager_get_ip(void) { return ip_str; }
