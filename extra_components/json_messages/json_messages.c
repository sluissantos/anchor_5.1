#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "config.h"
#include "esp_system.h"
#include "json_messages.h"
#include "config.h"
#include "https_ota.h"
#include <time.h>
#include "esp_http_client.h"
#include "dwm1001_main.h"
//#include "./../mqtts/mqtt_headers.h"

#define TAG "JSON_MESSAGES"

#define HTTP_AUTH_TYPE_DEFAULT HTTP_AUTH_TYPE_BASIC

extern bool synchronized_time;

struct nvs_data_t nvs_data_cjson;

int json_firmware_update_cmd_recv(char *buf) {
	nvs_handle config_handle;
	nvs_data_cjson = getNvsData();

	cJSON *root = cJSON_Parse(buf);

	cJSON *server = cJSON_GetObjectItem(root, "server");

	cJSON *url = cJSON_GetObjectItem(server, "url");

	if (url) strcpy(nvs_data_cjson.fw_update_data.url, url->valuestring);
	else strcpy(nvs_data_cjson.fw_update_data.url,CONFIG_DEFAULT_FIRMWARE_UPGRADE_URI);

	//Procurar por HTTPS ou HTTP:
	char *pbuffer = NULL;
	pbuffer = (char*) strstr(nvs_data_cjson.fw_update_data.url, "http://");
	if (pbuffer != NULL) nvs_data_cjson.fw_update_data.protocol = HTTP; //HTTP detectado
	else nvs_data_cjson.fw_update_data.protocol = HTTPS; //HTTPS é default

	cJSON *cert = cJSON_GetObjectItem(server, "cert");
	
	if (cert) {
		nvs_open("nvs", NVS_READWRITE, &config_handle);
		nvs_erase_key(config_handle, "fw_update_cert");
		nvs_commit(config_handle);
		if (nvs_set_str(config_handle, "fw_update_cert", cert->valuestring)!= ESP_OK) {
			ESP_LOGE(TAG, "Certificate write error!");
		}
		nvs_commit(config_handle);
		nvs_close(config_handle);
	}

	cJSON *auth = cJSON_GetObjectItem(server, "auth");

	cJSON *auth_type = cJSON_GetObjectItem(auth, "type");
	if (auth_type) {
		if (strcmp("none", auth_type->valuestring) == 0)
			nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_NONE;
		else if (strcmp("basic", auth_type->valuestring) == 0)
			nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_BASIC;
		else if (strcmp("digest", auth_type->valuestring) == 0)
			nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_DIGEST;
		else {
			if (nvs_data_cjson.fw_update_data.protocol == HTTPS)
				nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_DEFAULT;
			else
				nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_NONE;
		}
	} else if (nvs_data_cjson.fw_update_data.protocol == HTTPS)
		nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_DEFAULT;
	else
		nvs_data_cjson.fw_update_data.http_auth_type = HTTP_AUTH_TYPE_NONE;

	cJSON *user = cJSON_GetObjectItem(auth, "user");
	if (user) strcpy(nvs_data_cjson.fw_update_data.http_user, user->valuestring);
	else if (nvs_data_cjson.fw_update_data.protocol == HTTPS){
//		uint8_t wifi_sta_mac_addr[6];
//		esp_read_mac(wifi_sta_mac_addr, ESP_MAC_WIFI_STA);
//		sprintf(nvs_data.fw_update_data.http_user, MACSTRCAP,MAC2STR(wifi_sta_mac_addr));
		strcpy(nvs_data_cjson.fw_update_data.http_user, CONFIG_DEFAULT_FIRMWARE_UPGRADE_AUTH_USER);
	}
	else memset(nvs_data_cjson.fw_update_data.http_user, '\0', sizeof(nvs_data_cjson.fw_update_data.http_user));

	cJSON *pass = cJSON_GetObjectItem(auth, "pass");
	if (pass) strcpy(nvs_data_cjson.fw_update_data.http_pass, pass->valuestring);
	else if (nvs_data_cjson.fw_update_data.protocol == HTTPS) {
		strcpy(nvs_data_cjson.fw_update_data.http_pass, CONFIG_DEFAULT_FIRMWARE_UPGRADE_AUTH_PASSWORD);
	} else memset(nvs_data_cjson.fw_update_data.http_pass, '\0', sizeof(nvs_data_cjson.fw_update_data.http_pass));

	cJSON *attempts = cJSON_GetObjectItem(server, "attempts");
	if (attempts) nvs_data_cjson.fw_update_data.attempts = attempts->valueint;
	else nvs_data_cjson.fw_update_data.attempts = CONFIG_DEFAULT_FIRMWARE_UPGRADE_ATTEMPTS;

	cJSON *timeout = cJSON_GetObjectItem(server, "timeout");
	if (timeout) nvs_data_cjson.fw_update_data.timeout = timeout->valueint;
	else nvs_data_cjson.fw_update_data.timeout = CONFIG_DEFAULT_FIRMWARE_UPGRADE_TIMEOUT;

	nvs_data_cjson.fw_update_data.ret = UPDATE_SCHEDULED;

	cJSON_Delete(root);

	nvs_open("nvs", NVS_READWRITE, &config_handle);
	size_t size = sizeof(nvs_data_cjson.fw_update_data);
	nvs_set_blob(config_handle, "fw_update_data", &nvs_data_cjson.fw_update_data, size);
	nvs_commit(config_handle);

	nvs_close(config_handle);

	return nvs_data_cjson.fw_update_data.ret;
}
//lpx/bkm/94B97ED60654/down/cmd/upgrade
//{
//  "server": {
//    "url":"http://192.168.1.102:8080/firmware/v1.0.5/aura_gateway_esp.bin",
//    "auth": {
//      "type": "none"
//    },
//    "attempts": 3,
//    "timeout": 120
//  }
//}

char * json_firmware_update_return(int update_return){
	char * buf = malloc(128* sizeof(char));
	cJSON *fw_sched = cJSON_CreateObject();
	cJSON *update = cJSON_CreateObject();
	cJSON_AddItemToObject(fw_sched, "update",update);
	if(update_return==UPDATE_SCHEDULED)	cJSON_AddStringToObject(update,"status","starting");
		else if(update_return==UPDATE_SUCCESS) cJSON_AddStringToObject(update,"status","success");
		else {
			cJSON_AddStringToObject(update,"status","fail");
			cJSON_AddNumberToObject(update, "error",(signed int)update_return);
		}
	buf = cJSON_Print(fw_sched);
	cJSON_Delete(fw_sched);
	return buf;
}


char* json_dwm1001_alive_on(struct dmw1001_status_t * dmw1001_status_info, uint8_t spidev){
	char *buf = malloc(1024 * sizeof(char));
	cJSON *info_dwm1001 = cJSON_CreateObject();

	cJSON_AddStringToObject(info_dwm1001, "alive", "on");
	time_t now = 0;
	struct tm timeinfo = { 0 };
	if(synchronized_time){
		time(&now);
		localtime_r(&now, &timeinfo);
		cJSON_AddNumberToObject(info_dwm1001, "tmst",now);
	}
	char node_id_str[16];
	sprintf(node_id_str,"%04X",(uint16_t)dmw1001_status_info->node_id);
	cJSON_AddStringToObject(info_dwm1001, "id", node_id_str);
	cJSON_AddStringToObject(info_dwm1001, "label", (const char *)dmw1001_status_info->label);
	char panid_str[16];
	sprintf(panid_str,"%04X",dmw1001_status_info->panid);
	cJSON_AddStringToObject(info_dwm1001, "pan_id",panid_str);
	cJSON_AddNumberToObject(info_dwm1001, "dev_number",spidev);
	cJSON_AddNumberToObject(info_dwm1001, "mode", dmw1001_status_info->cfg_node.mode);
	cJSON *position = cJSON_CreateObject();
	cJSON_AddItemToObject(info_dwm1001, "dev_pos", position);
		cJSON_AddNumberToObject(position, "x",dmw1001_status_info->dev_pos.x);
		cJSON_AddNumberToObject(position, "y",dmw1001_status_info->dev_pos.y);
		cJSON_AddNumberToObject(position, "z",dmw1001_status_info->dev_pos.z);
		cJSON_AddNumberToObject(position, "qf",dmw1001_status_info->dev_pos.qf);
	cJSON_AddNumberToObject(info_dwm1001, "ble_en", dmw1001_status_info->cfg_node.common.ble_en);
	cJSON_AddNumberToObject(info_dwm1001, "fw_update_en", dmw1001_status_info->cfg_node.common.fw_update_en);
	cJSON_AddNumberToObject(info_dwm1001, "uwb_mode", dmw1001_status_info->cfg_node.common.uwb_mode);
	cJSON_AddNumberToObject(info_dwm1001, "led_en", dmw1001_status_info->cfg_node.common.led_en);
	cJSON_AddNumberToObject(info_dwm1001, "enc_en", dmw1001_status_info->cfg_node.common.enc_en);
	if(dmw1001_status_info->cfg_node.mode==DWM_MODE_TAG){
		cJSON_AddNumberToObject(info_dwm1001, "loc_engine_en", dmw1001_status_info->cfg_node.loc_engine_en);
		cJSON_AddNumberToObject(info_dwm1001, "low_power_en", dmw1001_status_info->cfg_node.loc_engine_en);
		cJSON_AddNumberToObject(info_dwm1001, "stnry_en", dmw1001_status_info->cfg_node.stnry_en);
		cJSON_AddNumberToObject(info_dwm1001, "meas_mode", dmw1001_status_info->cfg_node.meas_mode);
	}
	else{ //DWM_MODE_ANCHOR
		cJSON_AddNumberToObject(info_dwm1001, "initiator", dmw1001_status_info->cfg_node.initiator);
		cJSON_AddNumberToObject(info_dwm1001, "bridge", dmw1001_status_info->cfg_node.bridge);
		cJSON_AddNumberToObject(info_dwm1001, "uwb_bh_routing", dmw1001_status_info->cfg_node.uwb_bh_routing);
	}
	buf = cJSON_Print((const cJSON*)info_dwm1001);
	cJSON_Delete(info_dwm1001);
	return buf;
}

char* json_dwm1001_alive_off(void) {
	char *buf = malloc(256 * sizeof(char));
	cJSON *alive = cJSON_CreateObject();

	cJSON_AddStringToObject(alive, "alive", "off");

	time_t now = 0;
	struct tm timeinfo = { 0 };
	if(synchronized_time){
		time(&now);
		localtime_r(&now, &timeinfo);
		cJSON_AddNumberToObject(alive, "tmst",now);
	}

	buf = cJSON_Print((const cJSON*)alive);
	cJSON_Delete(alive);
	return buf;
}
extern struct dmw1001_status_t dmw1001_status[NUM_OF_DWM1001_MODULES];
void json_cmd_config_uwb(char *buf, bool is_broadcast) {
	uint8_t spidev;
	dwm_pos_t pos;
	dwm_cfg_anchor_t cfg_an;
	cJSON *root = cJSON_Parse(buf);
	cJSON *id = cJSON_GetObjectItem(root, "id");

	cJSON *pan_id = cJSON_GetObjectItem(root, "pan_id");
	cJSON *dev_pos = cJSON_GetObjectItem(root, "dev_pos");
		cJSON *dev_pos_x = cJSON_GetObjectItem(dev_pos, "x");
		cJSON *dev_pos_y = cJSON_GetObjectItem(dev_pos, "y");
		cJSON *dev_pos_z = cJSON_GetObjectItem(dev_pos, "z");
	cJSON *ble_en = cJSON_GetObjectItem(root, "ble_en");
	cJSON *initiator = cJSON_GetObjectItem(root, "initiator");

	if(is_broadcast && !id){
		for(spidev=0;spidev<NUM_OF_DWM1001_MODULES;spidev++){
			if(pan_id) dwm_panid_set(&spidev,(uint16_t)strtol(pan_id->valuestring, NULL, 16));
			if(dev_pos && dev_pos_x && dev_pos_y && dev_pos_z) {
				pos.x=dev_pos_x->valueint;
				pos.y=dev_pos_y->valueint;
				pos.z=dev_pos_z->valueint;
				pos.qf=100;
				dwm_pos_set(&spidev, &pos);
			}
			if(ble_en || initiator){
				if(initiator) cfg_an.initiator = (bool)initiator->valueint;
				else cfg_an.initiator = dmw1001_status[spidev].cfg_node.initiator;
				cfg_an.bridge = dmw1001_status[spidev].cfg_node.bridge;
				cfg_an.uwb_bh_routing = dmw1001_status[spidev].cfg_node.uwb_bh_routing;
				cfg_an.common.enc_en = dmw1001_status[spidev].cfg_node.common.enc_en;
				cfg_an.common.led_en = dmw1001_status[spidev].cfg_node.common.led_en;
				if(ble_en) cfg_an.common.ble_en = (bool)ble_en->valueint;
				else cfg_an.common.ble_en = dmw1001_status[spidev].cfg_node.common.ble_en;
				cfg_an.common.uwb_mode = dmw1001_status[spidev].cfg_node.common.uwb_mode;
				cfg_an.common.fw_update_en = dmw1001_status[spidev].cfg_node.common.fw_update_en;
				dwm_cfg_anchor_set(&spidev,&cfg_an);
			}
		}
		cJSON_Delete(root);
		return;
	}
	if(!id) return;
	spidev = dwm1001_search_device_by_id(id->valuestring);
	if(spidev==DWM1001_SPIDEV_ID_NOT_FOUND) {
		ESP_LOGE(TAG, "UWB ID not found!");
		return; //Not found
	}
	if(pan_id) dwm_panid_set(&spidev,(uint16_t)strtol(pan_id->valuestring, NULL, 16));
	if(dev_pos && dev_pos_x && dev_pos_y && dev_pos_z) {
		pos.x=dev_pos_x->valueint;
		pos.y=dev_pos_y->valueint;
		pos.z=dev_pos_z->valueint;
		pos.qf=100;
		dwm_pos_set(&spidev, &pos);
	}
	if(ble_en || initiator){
		if(initiator) cfg_an.initiator = (bool)initiator->valueint;
		else cfg_an.initiator = dmw1001_status[spidev].cfg_node.initiator;
		cfg_an.bridge = dmw1001_status[spidev].cfg_node.bridge;
		cfg_an.uwb_bh_routing = dmw1001_status[spidev].cfg_node.uwb_bh_routing;
		cfg_an.common.enc_en = dmw1001_status[spidev].cfg_node.common.enc_en;
		cfg_an.common.led_en = dmw1001_status[spidev].cfg_node.common.led_en;
		if(ble_en) cfg_an.common.ble_en = (bool)ble_en->valueint;
		else cfg_an.common.ble_en = dmw1001_status[spidev].cfg_node.common.ble_en;
		cfg_an.common.uwb_mode = dmw1001_status[spidev].cfg_node.common.uwb_mode;
		cfg_an.common.fw_update_en = dmw1001_status[spidev].cfg_node.common.fw_update_en;
		dwm_cfg_anchor_set(&spidev,&cfg_an);
	}
	cJSON_Delete(root);
}

#include "esp_wifi.h"
void json_cmd_config_wifi(char *buf) {
	cJSON *root = cJSON_Parse(buf);

	cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
	cJSON *pwd = cJSON_GetObjectItemCaseSensitive(root, "pwd");

	cJSON *broker_uri = cJSON_GetObjectItemCaseSensitive(root, "broker_uri");
	cJSON *broker_user = cJSON_GetObjectItemCaseSensitive(root, "broker_user");
	cJSON *broker_pwd = cJSON_GetObjectItemCaseSensitive(root, "broker_pwd");
	wifi_config_t sta_config;
	bool save_changes=false;
	bool wifi_changes=false;

	if(broker_uri) {
		nvs_handle config_handle;
		nvs_open("nvs", NVS_READWRITE, &config_handle);
		nvs_erase_key(config_handle, "fw_uri_broker");
		nvs_commit(config_handle);

		if (nvs_set_str(config_handle, "fw_uri_broker", broker_uri->valuestring)!= ESP_OK) {
			ESP_LOGE(TAG, "Uri Broker write error!");
		}

		nvs_commit(config_handle);
		nvs_close(config_handle);
		save_changes=true;
	}

	if(broker_user) {
		nvs_handle config_handle;
		nvs_open("nvs", NVS_READWRITE, &config_handle);
		nvs_erase_key(config_handle, "fw_user_broker");
		nvs_commit(config_handle);

		if (nvs_set_str(config_handle, "fw_user_broker", broker_user->valuestring)!= ESP_OK) {
			ESP_LOGE(TAG, "User Broker write error!");
		}

		nvs_commit(config_handle);
		nvs_close(config_handle);
		save_changes=true;
	}

	if(broker_pwd) {
		nvs_handle config_handle;
		nvs_open("nvs", NVS_READWRITE, &config_handle);
		nvs_erase_key(config_handle, "fw_pwd_broker");
		nvs_commit(config_handle);

		if (nvs_set_str(config_handle, "fw_pwd_broker", broker_pwd->valuestring)!= ESP_OK) {
			ESP_LOGE(TAG, "Pwd Broker write error!");
		}

		nvs_commit(config_handle);
		nvs_close(config_handle);
		save_changes=true;
	}

	if(ssid) {
		uint8_t ssid_len = strlen(ssid->valuestring);
		strncpy((char *)sta_config.sta.ssid, ssid->valuestring, ssid_len*sizeof(char));
		sta_config.sta.ssid[ssid_len] = '\0';
		save_changes=true;
		wifi_changes=true;
	}

	if(pwd){
		uint8_t pwd_len = strlen(pwd->valuestring);
		strncpy((char *)sta_config.sta.password, pwd->valuestring, pwd_len*sizeof(char));
		sta_config.sta.password[pwd_len] = '\0';
		save_changes=true;
		wifi_changes=true;
	}

	if(save_changes==true) {

		if(wifi_changes==true){

			esp_wifi_set_mode(WIFI_MODE_STA);
			sta_config.sta.bssid_set=0;
			esp_err_t err= esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		
			if(err==ESP_OK) {
				vTaskDelay(pdMS_TO_TICKS(100));
				esp_restart();
			}
		} else {
			vTaskDelay(pdMS_TO_TICKS(100));
			esp_restart();
		}
	}
	cJSON_Delete(root);
}
//lpx/bkm/broad/down/cmd/wifi
//{
//"ssid": "Logpyx-POC",
//"pwd": "128Parsecs!"
//}
