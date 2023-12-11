#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_spiffs.h"

#include "build_defs.h"
#include "blufi_example.h"
#include "config.h"
//#include "https_ota.h"
#include "esp_ota_ops.h"
#include "status.h"
#include "esp_mac.h"

#define TAG "CONFIG"

struct nvs_data_t nvs_data;
struct factory_config_t factory_config;
nvs_handle_t config_factory_handle;

static void pairing_default(void){
	static wifi_config_t sta_config;

	ESP_LOGI(TAG, "Pairing Default Values... ");
	esp_wifi_disconnect();

	nvs_handle_t config_handle;
	nvs_open("nvs", NVS_READWRITE, &config_handle);
	nvs_erase_key(config_handle, "fw_uri_broker");
	nvs_erase_key(config_handle, "fw_user_broker");
	nvs_erase_key(config_handle, "fw_pwd_broker");
	nvs_commit(config_handle);

	if (nvs_set_str(config_handle, "fw_uri_broker", DEFAULT_BROKER_URI)!= ESP_OK) {
		ESP_LOGE(TAG, "Uri Broker write error!");
	} else if (nvs_set_str(config_handle, "fw_user_broker", DEFAULT_BROKER_USER)!= ESP_OK) {
		ESP_LOGE(TAG, "User Broker write error!");
	} else if (nvs_set_str(config_handle, "fw_pwd_broker", DEFAULT_BROKER_PWD)!= ESP_OK){
		ESP_LOGE(TAG, "pwd Broker write error!");
	}

	nvs_commit(config_handle);
	nvs_close(config_handle);

	strcpy((char *)sta_config.sta.ssid, DEFAULT_SSID);
	strcpy((char *)sta_config.sta.password, DEFAULT_PASSWORD);

	esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) setting default SSID/Password!", esp_err_to_name(err));
	}
	else ESP_LOGI(TAG, "Default SSID/Password saved!!!");

	esp_wifi_connect();
}

void reparing_wifi(void){
	pairing_default();
	esp_restart();
}

esp_err_t config_base_mac(void){
	esp_err_t err;
	if(factory_config.mem_read_err == ESP_OK){
		//MAC:
		if(factory_config.mac[0]==0 && factory_config.mac[1]==0 && factory_config.mac[2]==0 && factory_config.mac[3]==0 && factory_config.mac[4]==0 && factory_config.mac[5]==0){
			ESP_LOGW(TAG, "MAC READ OF NVS IS EMPTY!");
			err=ESP_ERR_INVALID_MAC;
		}
		else{
			ESP_LOGI(TAG, "Writing Base MAC Address ... ");
			err = esp_iface_mac_addr_set(factory_config.mac, ESP_MAC_BASE);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Error (%s) setting base MAC!", esp_err_to_name(err));
			}
		}
	}
	else return factory_config.mem_read_err;
	return err;
}

esp_err_t config_hostname(void){
	esp_err_t err = NULL;
	/*
	char hostname[64];

	if(factory_config.mem_read_err == ESP_OK){
    uint8_t wifi_sta_mac_addr[6];
	err = esp_wifi_get_mac(ESP_IF_WIFI_STA, wifi_sta_mac_addr);
		//HOSTNAME:
		ESP_LOGI(TAG, "Writing Hostname ... ");
		sprintf(hostname,""HOSTNAME_PREFIX_WIFI"-%02X%02X%02X",wifi_sta_mac_addr[3],wifi_sta_mac_addr[4],wifi_sta_mac_addr[5]);
		err = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error (%s) setting Hostname", esp_err_to_name(err));
			return err;
		}
	}
	else {
		return factory_config.mem_read_err;
	}
	*/
	ESP_LOGI("CONFIG", "REMOVEU AQUI");
	return err;
}

esp_err_t factory_init_data(void){
	esp_err_t err;

	err = nvs_open_from_partition(NVS_FACTORY_PARTITION,NVS_FACTORY_NAMESPACE, NVS_READWRITE, &config_factory_handle);
	factory_config.mem_read_err = err;
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) opening Factory NVS handle!", esp_err_to_name(err));
		return err;
	} else {
		ESP_LOGI(TAG, "Done");
		// Read data
		//--MANUFACTURER--
		char *buf = malloc( 32 * sizeof(char));
		size_t buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "manufacturer", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.manufacturer,buf);
			ESP_LOGI(TAG, "MANUFACTURER: %s", factory_config.manufacturer);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--HARDWARE VERSION--
		err = nvs_get_u16(config_factory_handle, "id_plat", &factory_config.id_plat);
		err = nvs_get_u8(config_factory_handle, "hw_major", &factory_config.hw_major);
		err = nvs_get_u8(config_factory_handle, "hw_minor", &factory_config.hw_minor);
		err = nvs_get_u8(config_factory_handle, "hw_rev", &factory_config.hw_rev);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "HARDWARE: platform: %d, version: %d.%d.%d", factory_config.id_plat, factory_config.hw_major,factory_config.hw_minor,factory_config.hw_rev);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));

		//--PLATFORM NAME--
		buf = malloc( 32 * sizeof(char));
		buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "plat_name", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.plat_name,buf);
			ESP_LOGI(TAG, "PLATAFORM NAME: %s", factory_config.plat_name);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--FIRMWARE BASE VERSION--
		err = nvs_get_u8(config_factory_handle, "fw_base_major", &factory_config.fw_base_major);
		err = nvs_get_u8(config_factory_handle, "fw_base_minor", &factory_config.fw_base_minor);
		err = nvs_get_u8(config_factory_handle, "fw_base_rev", &factory_config.fw_base_rev);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "FIRMWARE BASE VERSION: %d.%d.%d", factory_config.fw_base_major,factory_config.fw_base_minor,factory_config.fw_base_rev);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));

		//--TESTED FLAG--
		err = nvs_get_u8(config_factory_handle, "test_ok", &factory_config.test_ok);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "TESTES: %d", factory_config.test_ok);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));

		//--WARRANTY DATETIME--
		err = nvs_get_u32(config_factory_handle, "wty_datetime", &factory_config.wty_datetime);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "WARRANTY DATETIME: %ld", factory_config.wty_datetime);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));

		//--QRCODE--
		buf = malloc( 64 * sizeof(char));
		buf_len = 64 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "qr_code", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.qr_code,buf);
			ESP_LOGI(TAG, "QR CODE: %s", factory_config.qr_code);
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--MAC--
		buf = malloc( 32 * sizeof(char));
		buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "mac", buf,&buf_len);
		if(err == ESP_OK){
			sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &factory_config.mac[0], &factory_config.mac[1], &factory_config.mac[2], &factory_config.mac[3], &factory_config.mac[4], &factory_config.mac[5]);
			ESP_LOGI(TAG, "MAC: " MAC2STRCAP "", MAC2STR(factory_config.mac));
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--FACTORY NEW FLAG--
		err = nvs_get_u8(config_factory_handle, "fact_new", &factory_config.fact_new);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "FACTORY NEW FLAG: %d", factory_config.fact_new);
		}
		else {
			factory_config.fact_new=0;
			ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		}

		err = nvs_get_u16(config_factory_handle, "id_factory", &factory_config.id_factory);

		factory_config.id_factory = CONFIG_PRODUCT_ID;
		ESP_LOGI(TAG, "ID FACTORY: %d", factory_config.id_factory);

		//--PROD BY--
		buf = malloc( 32 * sizeof(char));
		buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "prod_by", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.prod_by,buf);
			ESP_LOGI(TAG, "PROD BY: %s", factory_config.prod_by);
			//ESP_LOGI(TAG, "Done");
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--MANUFACTURE BATCH--
		//ESP_LOGI(TAG, "Reading MANUFACTURE BATCH from Factory NVS ... ");
		buf = malloc( 32 * sizeof(char));
		buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "mfg_batch", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.mfg_batch,buf);
			ESP_LOGI(TAG, "MANUFACTURE BATCH: %s", factory_config.mfg_batch);
			//ESP_LOGI(TAG, "Done");
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--MANUFACTURE LOCAL--
		buf = malloc( 32 * sizeof(char));
		buf_len = 32 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "mfg_local", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.mfg_local,buf);
			ESP_LOGI(TAG, "MANUFACTURE LOCAL: %s", factory_config.mfg_local);
			//ESP_LOGI(TAG, "Done");
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		//--MANUFACTURE DATETIME--
		err = nvs_get_u32(config_factory_handle, "mfg_datetime", &factory_config.mfg_datetime);
		if(err == ESP_OK){
			ESP_LOGI(TAG, "MANUFACTURE DATETIME: %ld", factory_config.mfg_datetime);
			//ESP_LOGI(TAG, "Done");
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));

		//--OPERATOR NAME--
		//ESP_LOGI(TAG, "Reading OPERATOR NAME from Factory NVS ... ");
		buf = malloc( 64 * sizeof(char));
		buf_len = 64 * sizeof(char);
		err = nvs_get_str(config_factory_handle, "op_name", buf,&buf_len);
		if(err == ESP_OK){
			strcpy(factory_config.op_name,buf);
			ESP_LOGI(TAG, "OPERATOR NAME: %s", factory_config.op_name);
			//ESP_LOGI(TAG, "Done");
		}
		else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		free(buf);

		// Close
		nvs_close(config_factory_handle);
	}

	return ESP_OK;
}

esp_err_t config_load(void){
	esp_err_t err = ESP_OK;
	size_t size;
	esp_app_desc_t running_app_info;
	const esp_partition_t *running = esp_ota_get_running_partition();

	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
			//ESP_LOGI(TAG, "\r\nProject name: %s\r\nProduct name: %s (id:%d)\r\nFirmware version: %d.%d.%d\r\nCommit: %s\r\nDatetime: %s,%s\r\nIDF version: %s\r\nPartition name: %s\r\nEncrypted:%d", running_app_info.project_name, HOSTNAME_PREFIX, CONFIG_PRODUCT_ID, CONFIG_FIRMWARE_VERSION_MAJOR,CONFIG_FIRMWARE_VERSION_MINOR,CONFIG_FIRMWARE_VERSION_REVISION, running_app_info.version,running_app_info.date,running_app_info.time,running_app_info.idf_ver, running->label,running->encrypted);
	}

//	const esp_app_desc_t* running_app_info = esp_ota_get_app_description();
//	ESP_LOGI(TAG, "\r\nProject name: %s\r\nFirmware Version: %s\r\nCommit: %s\r\nDatetime: %s,%s\r\nIDF version: %s", FIRMWARE_VERSION, running_app_info->project_name,running_app_info->version,running_app_info->date,running_app_info->time,running_app_info->idf_ver);
	// Open
	ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");
	nvs_handle_t config_handle;
	err = nvs_open("nvs", NVS_READWRITE, &config_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
		return err;
	} else {
		ESP_LOGI(TAG, "Done");
		ESP_LOGI(TAG, "Reading cfg_holder from NVS ... ");
		nvs_data.cfg_holder = 0; // value will default to 0, if not set yet in NVS
		err = nvs_get_u32(config_handle, "cfg_holder", &nvs_data.cfg_holder);
		switch (err) {
			case ESP_OK:
				ESP_LOGI(TAG, "Done");
				ESP_LOGI(TAG, "cfg_holder = %ld", nvs_data.cfg_holder);

				//Reading Timezone
				char *buf = malloc( 64 * sizeof(char));
				size_t buf_len = 64 * sizeof(char);
				err = nvs_get_str(config_handle, "timezone", buf,&buf_len);
				if(err == ESP_OK){
					strcpy(nvs_data.timezone,buf);
					ESP_LOGI(TAG, "Timezone: %s", nvs_data.timezone);
				}
				else ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
				free(buf);
				//
				size = sizeof(nvs_data.fw_update_data);
				nvs_get_blob(config_handle, "fw_update_data", &nvs_data.fw_update_data, &size);
	            if(nvs_data.fw_update_data.ret!=ESP_OK) ESP_LOGE(TAG, "UPDATE RETURN: 0x%02x",nvs_data.fw_update_data.ret); //ESP_LOGE(TAG, "UPDATE RETURN: %s",esp_err_to_name(nvs_data.fw_update_data.ret));

				break;
			case ESP_ERR_NVS_NOT_FOUND:
				//Primeira vez que o código roda no módulo (inserir MAC e rede Padrão)
				ESP_LOGW(TAG, "The value is not initialized yet!");
				//first_round();
				pairing_default();

				//Set default timezone (GMT0):
				memset(nvs_data.timezone, 0, sizeof(nvs_data.timezone));
				strcpy(nvs_data.timezone,"GMT0");
				nvs_set_str(config_handle, "timezone", (const char *)&nvs_data.timezone);

				ESP_LOGI(TAG, "Committing update_times in NVS ... ");
				err = nvs_commit(config_handle);
				if (err != ESP_OK) {
					ESP_LOGE(TAG, "NVS update_times Commit Failed!");
					return err;
				}
				else ESP_LOGI(TAG, "NVS update_times Commit Done");
				break;
			default :
				ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
				return err;
		}

		//Novo firmware. (pós update) Deve permanecer as configurações de MAC e rede. Pode ter atualizado o firmware para um outro produto.
		if(nvs_data.cfg_holder != COMPILE_ID) {
			// Write
			ESP_LOGI(TAG, "Updating cfg_holder in NVS ... ");
			//cfg_holder++;
			err = nvs_set_u32(config_handle, "cfg_holder", COMPILE_ID);
			if (err != ESP_OK) ESP_LOGE(TAG, "Failed!");
			else ESP_LOGI(TAG, "Done");

			ESP_LOGI(TAG, "Committing cfg_holder in NVS ... ");
			err = nvs_commit(config_handle);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "NVS cfg_holder Commit Failed!");
				return err;
			}
			else ESP_LOGI(TAG, "NVS cfg_holder Commit Done");
		}

		// Close
		nvs_close(config_handle);

	}

	return err;
}

esp_err_t factory_reset(void){
	esp_err_t err;
	err=nvs_flash_erase();
	if (err != ESP_OK) 	{
		ESP_LOGE(TAG, "Error (%s) erasing NVS!", esp_err_to_name(err));
	}
	err=esp_spiffs_format("storage");
	if (err != ESP_OK) 	{
			ESP_LOGE(TAG, "Error (%s) erasing SPIFFS!", esp_err_to_name(err));
	}
	return err;
}

struct nvs_data_t getNvsData(){
	return nvs_data;
}

struct factory_config_t getFactoryConfig(){
	return factory_config;
}