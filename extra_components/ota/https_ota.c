#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "status.h"
#include "config.h"
#include "ledc_headers.h"
#include "https_ota.h"
#include "esp_mac.h"
#include "blufi_example.h"
static const char *TAG = "HTTPS OTA";

extern const uint8_t root_ca_pem_start[] asm("_binary_root_ca_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_root_ca_pem_end");

EventGroupHandle_t status_event_group_https;
struct nvs_data_t nvs_data_https;
extern const int CONNECTED_BIT;

esp_err_t _http_event_handler(esp_http_client_event_t *evt){
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT, data_len=%d, value=%s",evt->data_len, (char *)evt->data);
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}


static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

static TimerHandle_t tmr;
static bool timeout_flag=false;
static void ota_timeout_cb ( TimerHandle_t xTimer )
{
	ESP_LOGW(TAG, "OTA TIMEOUT");
	timeout_flag=true;
}

#define DEFAULT_OTA_TIMEOUT 60000
static esp_err_t esp_https_ota_process(const esp_http_client_config_t *config, uint32_t ota_timeout){
    if (!config) {
        ESP_LOGE(TAG, "esp_http_client config not found");
        return ESP_ERR_INVALID_ARG;
    }

    esp_https_ota_config_t ota_config = {
        .http_config = config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (https_ota_handle == NULL) {
        return ESP_FAIL;
    }

    if(tmr!=NULL) xTimerStop(tmr, 0);
    timeout_flag=false;
    tmr = xTimerCreate("OtaTimer", pdMS_TO_TICKS(ota_timeout), pdFALSE, ( void * )https_ota_handle, &ota_timeout_cb);
    if( xTimerStart(tmr, 10 ) != pdPASS ) {
    	ESP_LOGE(TAG,"Timer start error\n");
    	return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA TIMER INIT");

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if(timeout_flag) err=ESP_ERR_TIMEOUT;

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if(tmr!=NULL) xTimerStop(tmr, 0);
    timeout_flag=false;
    tmr=NULL;

    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if (err != ESP_OK) {
        /* If there was an error in esp_https_ota_perform(),
           then it is given more precedence than error in esp_https_ota_finish()
         */
        return err;
    } else if (ota_finish_err != ESP_OK) {
    	return ota_finish_err;
    }
    return ESP_OK;
}

static void ota_task(void * pvParameter){
	size_t size;
	nvs_handle_t config_handle;
	char * cert_str = NULL;

	struct fw_update_t * fw_update = (struct fw_update_t *)pvParameter;

	ESP_LOGI(TAG, "Starting OTA...");

    xEventGroupWaitBits(getWifiEventGroup(), CONNECTED_BIT,false, true, portMAX_DELAY);

    if(status_event_group_https != NULL)
        xEventGroupSetBits(status_event_group_https, STATUS_FWUPDATE_BIT);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Connected to WiFi network! Attempting to connect to server...");

    uint8_t wifi_sta_mac_addr[6];
    esp_read_mac(wifi_sta_mac_addr, ESP_MAC_WIFI_STA);

	nvs_open("nvs", NVS_READWRITE, &config_handle);
	cert_str = malloc(2048*sizeof(char));
	size_t cert_str_len = 2048*sizeof(char);
	nvs_get_str(config_handle, "fw_update_cert", cert_str, &cert_str_len);
	nvs_close(config_handle);

    //Pra não expor a senha Default, ela só será usada se estiver em HTTPS:
    esp_http_client_config_t config;
    if((fw_update->http_auth_type==HTTP_AUTH_TYPE_BASIC || fw_update->http_auth_type==HTTP_AUTH_TYPE_DIGEST) && (fw_update->protocol == HTTPS)){
        config = (esp_http_client_config_t){
                .url = fw_update->url,
                .event_handler = _http_event_handler,
                .timeout_ms = 10000,
                .username=(const char *)fw_update->http_user,
                .password=(const char *)fw_update->http_pass,
                .auth_type= fw_update->http_auth_type,//É possível usar Basic Auth ou Digest ou None

            };
		if(strncmp(cert_str,"-----BEGIN CERTIFICATE-----",27)==0) {
			ESP_LOGI(TAG,"Certificate inside NVS");
			config.cert_pem = (char *)cert_str;
		}
		else config.cert_pem = (const char *)root_ca_pem_start;
            ESP_LOGI(TAG, "Authenticated Mode:\r\nURL: %s\r\nProtocol: HTTPS\r\nAuth Type: %d\r\nUser: %s\r\nPass:%s", config.url, config.auth_type, config.username, config.password);
    }
    else{
    	config = (esp_http_client_config_t){
			.url = fw_update->url,
			.event_handler = _http_client_init_cb,
			.timeout_ms = 60000,
		};
		if(strncmp(cert_str,"-----BEGIN CERTIFICATE-----",27)==0){
			ESP_LOGI(TAG,"Certificate inside NVS");
			config.cert_pem = (char *)cert_str;
		}
		else config.cert_pem = (char *)root_ca_pem_start;
		    ESP_LOGI(TAG, "No Authenticated Mode:\r\nURL: %s",config.url);
    };

    esp_err_t err = ESP_OK;
    nvs_data_https = getNvsData();
    nvs_data_https.fw_update_data.ret = UPDATE_ABORTED;
    while(nvs_data_https.fw_update_data.attempts>0){
    	ESP_LOGI(TAG, "ESP_HTTPS_OTA, Attempt #%d ...",nvs_data_https.fw_update_data.attempts);
    	nvs_open("nvs", NVS_READWRITE, &config_handle);
    	nvs_data_https.fw_update_data.attempts--;
    	size = sizeof(nvs_data_https.fw_update_data);
    	nvs_set_blob(config_handle, "fw_update_data", &nvs_data_https.fw_update_data, size);
    	nvs_commit(config_handle);
    	nvs_close(config_handle);
    	vTaskDelay(100 / portTICK_PERIOD_MS);

    	err = esp_https_ota_process(&config,nvs_data_https.fw_update_data.timeout*1000); //Tempo global

    	if(err!= ESP_OK){
    		ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed... err: 0x%02x", err);
    		vTaskDelay(2000 / portTICK_PERIOD_MS);
    	}
    	else break;
    }

    if(cert_str!=NULL) {
    	free(cert_str);
    	cert_str=NULL;
    }

    if(err==ESP_OK) {
    	ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
    	led_handler(LED_UPDATE_SUCCESS);
    	nvs_data_https.fw_update_data.ret = UPDATE_SUCCESS;
    }
    else {
    	ESP_LOGE(TAG, "Aborting...");
    	led_handler(LED_UPDATE_FAIL);
    	nvs_data_https.fw_update_data.ret = err;
    }

    nvs_data_https.fw_update_data.attempts=0;

    nvs_open("nvs", NVS_READWRITE, &config_handle);
	size = sizeof(nvs_data_https.fw_update_data);
	nvs_set_blob(config_handle, "fw_update_data", &nvs_data_https.fw_update_data, size);
	nvs_commit(config_handle);
	nvs_close(config_handle);
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	esp_restart();

    vTaskDelete(NULL);
}

void ota_init(struct fw_update_t * fw_update){
    status_event_group_https = getStatusEventGroup();
	xTaskCreate(&ota_task, "ota_task", 16384, fw_update , configMAX_PRIORITIES - 4, NULL);
}
