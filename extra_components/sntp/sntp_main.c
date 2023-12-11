#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"

static const char *TAG = "SNTP";

struct factory_config_t factory_config_sntp;
struct nvs_data_t nvs_data_sntp;

bool synchronized_time=0;
void time_sync_notification_cb(struct timeval *tv){
    factory_config_sntp = getFactoryConfig();
	synchronized_time=1;
	ESP_LOGI(TAG, "Notification of a time synchronization event!");
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
    if(factory_config_sntp.wty_datetime==0){
        nvs_handle config_factory_handle;
        nvs_open_from_partition(NVS_FACTORY_PARTITION,NVS_FACTORY_NAMESPACE, NVS_READWRITE, &config_factory_handle);
        factory_config_sntp.wty_datetime = now;
        nvs_set_u32(config_factory_handle, "wty_datetime", factory_config_sntp.wty_datetime);
        nvs_commit(config_factory_handle);
        nvs_close(config_factory_handle);
    }
}

static void sntp_disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	ESP_LOGI(TAG, "SNTP stop...");
	esp_sntp_stop();
	esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &sntp_disconnect_handler);
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP, &sntp_disconnect_handler);
}

static void sntp_connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	ESP_LOGI(TAG, "SNTP init...");
	esp_sntp_init();
	esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &sntp_disconnect_handler, NULL);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &sntp_disconnect_handler, NULL);

}


void sntp_main_init(void){
    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "0.br.pool.ntp.org");

    nvs_data_sntp = getNvsData();
    setenv("TZ", nvs_data_sntp.timezone, 1);
    tzset();
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sntp_connect_handler, NULL));
}
