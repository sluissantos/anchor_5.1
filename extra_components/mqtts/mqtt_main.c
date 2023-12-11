#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "mqtt_headers.h"
#include "config.h"
#include "status.h"
#include "ledc_headers.h"
#include "https_ota.h"
#include "json_messages.h"
#include "esp_wifi_types.h"


#define MIN_DEFAULT_ATTEMPT_NUMBER 8 //28 sec. each
#define MAX_DEFAULT_ATTEMPT_NUMBER 1024  //to 1024: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192...
static const char *TAG = "MQTTS";

extern const uint8_t ca_mqtt_broker_pem_start[]   asm("_binary_globalsign_root_ca_r2_pem_start");
extern const uint8_t ca_mqtt_broker_pem_end[]   asm("_binary_globalsign_root_ca_r2_pem_end");

esp_mqtt_client_handle_t client;
struct mqtt_conf_t mqtt_conf;
static int update_return=0;
static uint32_t retry=0;
static uint32_t attempt_restart=MIN_DEFAULT_ATTEMPT_NUMBER;
struct mqtt_broker_conf broker_conf;	

esp_err_t err;
EventGroupHandle_t status_event_group_mqtt;
uint8_t led_color_blink_mqtt;
struct nvs_data_t nvs_data_mqtt;

void mqtt_ota_feedback(int update_return){
	char * resp_upgrade_topic = NULL;
	char * jsonstr = NULL;
	jsonstr = json_firmware_update_return(update_return);
	resp_upgrade_topic = malloc(128*sizeof(char));
	sprintf(resp_upgrade_topic,"%s"MQTT_CMD_UPGRADE"",mqtt_conf.up_topic);
	esp_mqtt_client_publish(client, resp_upgrade_topic, jsonstr, strlen(jsonstr), QOS1, RETAIN_OFF);
	free(jsonstr);
	jsonstr=NULL;
	free(resp_upgrade_topic);
	resp_upgrade_topic = NULL;
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event){
    esp_mqtt_client_handle_t client = event->client;
	status_event_group_mqtt = getStatusEventGroup();
	led_color_blink_mqtt = getLedColorBlink();
    //char * online_msg_json = NULL;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			xEventGroupSetBits(status_event_group_mqtt, STATUS_MQTT_CONNECTED_BIT);
            esp_mqtt_client_publish(client, mqtt_conf.lwt_topic, ONLINE_MESSAGE, strlen(ONLINE_MESSAGE), QOS1, RETAIN_ON);
            esp_mqtt_client_subscribe(client, mqtt_conf.down_topic, QOS1);
            esp_mqtt_client_subscribe(client, mqtt_conf.broad_down_topic, QOS1);

			if(nvs_data_mqtt.fw_update_data.ret!=UPDATE_NOT_SCHEDULED){ //tem algo pra relatar
				mqtt_ota_feedback(nvs_data_mqtt.fw_update_data.ret);
				nvs_handle config_handle;
				nvs_open("nvs", NVS_READWRITE, &config_handle);
				nvs_data_mqtt.fw_update_data.ret=UPDATE_NOT_SCHEDULED;
				size_t size = sizeof(nvs_data_mqtt.fw_update_data);
				nvs_set_blob(config_handle, "fw_update_data", &nvs_data_mqtt.fw_update_data, size);
				nvs_commit(config_handle);
				nvs_close(config_handle);
			}

            if(retry>0){
		    	retry=0;
		    	attempt_restart=MIN_DEFAULT_ATTEMPT_NUMBER;
		    }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			xEventGroupSetBits(status_event_group_mqtt, STATUS_MQTT_DISCONNECTED_BIT);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			led_color_blink_mqtt=LED_BLINK_REMOTE_NETWORK;
			xEventGroupSetBits(status_event_group_mqtt, STATUS_BLINK_LED_ON_BIT);
            if(update_return == UPDATE_SCHEDULED) {
            	esp_restart();
            }
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            //uint8_t prefix_down_topic_len = strlen(mqtt_conf.down_topic)-2;
            //uint8_t prefix_broad_down_topic_len;
            uint8_t prefix_len=0;
            bool broad_messsage=false;

            if(memcmp(event->topic,mqtt_conf.down_topic,strlen(mqtt_conf.down_topic)-2)==0){
            	broad_messsage=false;
            	prefix_len=strlen(mqtt_conf.down_topic)-2;
            	ESP_LOGI(TAG, "Unicast Command");
            }
            else if(memcmp(event->topic,mqtt_conf.broad_down_topic,strlen(mqtt_conf.broad_down_topic)-2)==0) {
            	broad_messsage=true;
            	prefix_len=strlen(mqtt_conf.broad_down_topic)-2;
            	ESP_LOGI(TAG, "Broadcast Command");
            }
            else break;

            if(memcmp(event->topic + prefix_len,MQTT_CMD_UPGRADE,strlen(MQTT_CMD_UPGRADE))==0){
				event->data[event->data_len]='\0';
				//printf("CMD upgrade received: %s\n",event->data);
				update_return=json_firmware_update_cmd_recv(event->data);
				mqtt_ota_feedback(update_return);
			}
			else if(memcmp(event->topic + prefix_len,MQTT_CMD_UPGRADE,strlen(MQTT_CMD_UPGRADE))==0){
				event->data[event->data_len]='\0';
				//printf("CMD upgrade received: %s\n",event->data);
				update_return=json_firmware_update_cmd_recv(event->data);
				mqtt_ota_feedback(update_return);
			}
			else if(memcmp(event->topic + prefix_len,MQTT_CMD_REBOOT,strlen(MQTT_CMD_REBOOT))==0){
				esp_restart();
			}
			else if(memcmp(event->topic + prefix_len,MQTT_CMD_FACTORY_RESET,strlen(MQTT_CMD_FACTORY_RESET))==0){
				if(status_event_group_mqtt != NULL)
					xEventGroupSetBits(status_event_group_mqtt, STATUS_RESET_BIT);
				factory_reset();
				vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 SEG
				esp_restart();
			}
			else if(memcmp(event->topic + prefix_len,MQTT_CMD_CONFIG_UWB,strlen(MQTT_CMD_CONFIG_UWB))==0){
				json_cmd_config_uwb(event->data, broad_messsage);
			}
			else if(memcmp(event->topic + prefix_len,MQTT_CMD_CONFIG_WIFI,strlen(MQTT_CMD_CONFIG_WIFI))==0){
				//BROADCAST CONFIG UWB
				json_cmd_config_wifi(event->data);
			}
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGE(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }

            //tentativas a cada 28 segundos
            retry++;
			if(retry < 1) attempt_restart = MIN_DEFAULT_ATTEMPT_NUMBER;
			if((retry%attempt_restart) == 0) //2, 4, 8, 16, 32, 64, 128, 256, 384, 512, 640, 768 ...
			{
				if(retry>127) attempt_restart = MAX_DEFAULT_ATTEMPT_NUMBER;
				else attempt_restart = 2 * retry;

				ESP_LOGW(TAG, "Max number of attempts reached: %ld, restarting WIFI connection", retry);
				esp_wifi_disconnect();
				vTaskDelay(100 / portTICK_PERIOD_MS);
				esp_wifi_connect();
			}
            break;
		case MQTT_EVENT_BEFORE_CONNECT:
			ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
			break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    mqtt_event_handler_cb(event_data);
}



static void mqtt_app_disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	ESP_LOGI(TAG, "------------------------>mqtt_app_disconnect_handler");
	esp_mqtt_client_handle_t* client = (esp_mqtt_client_handle_t*) arg;
	if (*client) {
		ESP_LOGI(TAG, "Stopping MQTT client");
		esp_mqtt_client_stop(*client);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &mqtt_app_disconnect_handler);
		esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP, &mqtt_app_disconnect_handler);
		//*client = NULL;
	}
}

static void mqtt_app_connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	//	esp_mqtt_client_handle_t * client = (esp_mqtt_client_handle_t*) arg;
	//	if (*client) {
	//		ESP_LOGI(TAG, "Starting MQTT client");
	//		strcpy(client->uri,"teste");
	//		esp_mqtt_client_start(*client);
	//		esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &mqtt_app_disconnect_handler, client);
	//		esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &mqtt_app_disconnect_handler, client);
	//	}

	nvs_handle config_handle;
	nvs_open("nvs", NVS_READWRITE, &config_handle);
	size_t required_size;

	err= nvs_get_str(config_handle, "fw_uri_broker", NULL, &required_size);
	err= nvs_get_str(config_handle, "fw_uri_broker", (char *)broker_conf.uri, &required_size);

	err= nvs_get_str(config_handle, "fw_user_broker", NULL, &required_size);
	err= nvs_get_str(config_handle, "fw_user_broker", (char *)broker_conf.user, &required_size);

	err= nvs_get_str(config_handle, "fw_pwd_broker", NULL, &required_size);
	err= nvs_get_str(config_handle, "fw_pwd_broker", (char *)broker_conf.pwd, &required_size);

	//tcpip_adapter_ip_info_t ip_info;
	//tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
	//sprintf(broker_uri,"%d.%d.%d.124", IP2STR(&ip_info.ip));
	//sprintf(broker_uri,"mqtt://%d.%d.%d.124:%d", ip4_addr1_16(&ip_info.ip), ip4_addr2_16(&ip_info.ip), ip4_addr3_16(&ip_info.ip),1883);
	//sprintf(broker_uri,"mqtt://192.168.32.86:1883");

	const esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = broker_conf.uri,
		.credentials.client_id = mqtt_conf.client_id,
		.credentials.username = broker_conf.user,
		.credentials.authentication.password = broker_conf.pwd,
		.broker.verification.certificate = (const char *)ca_mqtt_broker_pem_start,
		.session.last_will.topic = mqtt_conf.lwt_topic,
		.session.last_will.msg = LWT_MESSAGE_OFFLINE,
		.session.last_will.msg_len= strlen(LWT_MESSAGE_OFFLINE),
		.session.last_will.qos = QOS1,
		.session.last_will.retain = RETAIN_ON,
		.session.keepalive = 60
	};
	nvs_close(config_handle);
	ESP_LOGI(TAG, "MQTT URL: %s", mqtt_cfg.broker.address.uri);
	memset(&client,0,sizeof(esp_mqtt_client_handle_t));
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

	if (client) {
		esp_mqtt_client_start(client);
		esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &mqtt_app_disconnect_handler, &client);
		esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &mqtt_app_disconnect_handler, &client);
	}

}

void mqtt_init(void){
	status_event_group_mqtt = getStatusEventGroup();
	led_color_blink_mqtt = getLedColorBlink();
	ESP_LOGI(TAG, "MQTT Startup..");

    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    uint8_t wifi_sta_mac_addr[6];
    esp_wifi_get_mac(WIFI_IF_STA, wifi_sta_mac_addr);

    // Configurar tópicos MQTT
    sprintf(mqtt_conf.client_id, PRODUCT_NAME "-" MAC2STRCAP, MAC2STR(wifi_sta_mac_addr));
    sprintf(mqtt_conf.lwt_topic, MQTT_TOPIC_MANUFACTURER "/" MQTT_TOPIC_PROJECT "/" MAC2STRCAP MQTT_LWTTOPIC, MAC2STR(wifi_sta_mac_addr));
    sprintf(mqtt_conf.down_topic, MQTT_TOPIC_MANUFACTURER "/" MQTT_TOPIC_PROJECT "/" MAC2STRCAP MQTT_DOWNTOPIC "/#", MAC2STR(wifi_sta_mac_addr));
    sprintf(mqtt_conf.up_topic, MQTT_TOPIC_MANUFACTURER "/" MQTT_TOPIC_PROJECT "/" MAC2STRCAP MQTT_UPTOPIC, MAC2STR(wifi_sta_mac_addr));
    sprintf(mqtt_conf.broad_down_topic, MQTT_TOPIC_MANUFACTURER "/" MQTT_TOPIC_PROJECT "/" MQTT_TOPIC_BROAD MQTT_DOWNTOPIC "/#");

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &mqtt_app_connect_handler, &client));
}