#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "config.h"
#include "dwm_api.h"
#include "driver/gpio.h"
#include "json_messages.h"
#include "mqtt_headers.h"
#include "dwm1001_main.h"
#include "status.h"

#define TAG "DWM1001_MAIN"

//#define DWM1001_MAIN_DEBUG

#if NUM_OF_DWM1001_MODULES==2
#define DWM1001_MODULE_0 0
#define DWM1001_MODULE_1 1

#elif NUM_OF_DWM1001_MODULES==1
#define DWM1001_MODULE_0 0
#endif

#define DWM1001_ATTEMPS_MAX 3
//#define DWM1001_INTERVAL_CHECK 1000 //ms
#define DWM1001_INTERVAL_CHECK 60000 //ms

#define PAN_ID 0xAABC
#define ON_STATUS 1
#define OFF_STATUS 0

extern uint32_t status;

extern esp_mqtt_client_handle_t client;
extern struct mqtt_conf_t mqtt_conf;

EventGroupHandle_t status_event_group_dwm;

uint16_t dwm1001_addr[NUM_OF_DWM1001_MODULES];

struct dmw1001_status_t dmw1001_status[NUM_OF_DWM1001_MODULES];

uint8_t dwm1001_search_device_by_id(char * node_id_search){
	char node_id_str[5];

	for(uint8_t i=0;i<NUM_OF_DWM1001_MODULES;i++){
		memset(node_id_str,0,sizeof(node_id_str));
		sprintf(node_id_str,"%04X",(uint16_t)dmw1001_status[i].node_id);
		if(strcmp(node_id_str,node_id_search)==0) return i;
	}
	return DWM1001_SPIDEV_ID_NOT_FOUND;//Not found
}

int dwm1001_param_init(uint8_t spidev){
	uint8_t label[16], len;
	uint64_t node_id=0;
	char new_label[16];
	int ret = RV_ERR;
	ret=dwm_node_id_get(&spidev, &node_id);
	dwm1001_addr[spidev] = (uint16_t)node_id;
	if(ret != RV_OK) return ret;
	memset(&label,0,16*sizeof(uint8_t));
	ret=dwm_label_read(&spidev, label, &len);
	if(ret != RV_OK) return ret;
	sprintf(new_label,"LX%04X",(uint16_t)node_id);
	if(strcmp((char *)label,new_label)!=0){
		dwm_label_write(&spidev, (uint8_t *)new_label, strlen(new_label));
		ESP_LOGI(TAG,"Writing new label..\n");
	}

	dwm_cfg_t cfg;
	ret=dwm_cfg_get(&spidev, &cfg);
	if(ret != RV_OK) return ret;
	if(cfg.mode != DWM_MODE_ANCHOR){
		dwm_cfg_anchor_t cfg_an;
		cfg_an.initiator = 0;
		cfg_an.bridge = 0;
		cfg_an.uwb_bh_routing = DWM_UWB_BH_ROUTING_AUTO;
		cfg_an.common.enc_en = 0;
		cfg_an.common.led_en = 0;
		cfg_an.common.ble_en = 1;
		cfg_an.common.uwb_mode = DWM_UWB_MODE_ACTIVE;
		cfg_an.common.fw_update_en = 0;
		dwm_cfg_anchor_set(&spidev,&cfg_an);
	}
	return ret;
}

void dwm1001_hw_rst(uint8_t spidev){
	#ifdef DWM1001_MODULE_0
	if(spidev==DWM1001_MODULE_0) {
		gpio_set_level(AURA1_RST_PIN,1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(AURA1_RST_PIN,0);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(AURA1_RST_PIN,1);
	}
	#endif
	#ifdef DWM1001_MODULE_1
	if(spidev==DWM1001_MODULE_1){
		gpio_set_level(AURA2_RST_PIN,1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(AURA2_RST_PIN,0);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(AURA2_RST_PIN,1);
	}
	#endif
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void dwm1001_send_alive_off(uint16_t node_id){
	char uwb_alive_topic[128];
	char * dmw1001_alive_json = NULL;
	sprintf(uwb_alive_topic,"%s"MQTT_UWBTOPIC"/%04X/status",mqtt_conf.up_topic,(uint16_t)node_id);
	dmw1001_alive_json = json_dwm1001_alive_off();
	esp_mqtt_client_publish(client, uwb_alive_topic, dmw1001_alive_json, strlen(dmw1001_alive_json), QOS1, RETAIN_ON);
	free(dmw1001_alive_json);
	dmw1001_alive_json = NULL;
}

void dwm1001_send_alive_on(struct dmw1001_status_t * dmw1001_status_info,uint8_t spidev){
	char uwb_alive_topic[128];
	char * dmw1001_alive_json = NULL;
	sprintf(uwb_alive_topic,"%s"MQTT_UWBTOPIC"/%04X/status",mqtt_conf.up_topic,(uint16_t)dmw1001_status_info->node_id);
	dmw1001_alive_json = json_dwm1001_alive_on(dmw1001_status_info, spidev);
	esp_mqtt_client_publish(client, uwb_alive_topic, dmw1001_alive_json, strlen(dmw1001_alive_json), QOS1, RETAIN_ON);
	free(dmw1001_alive_json);
	dmw1001_alive_json = NULL;
}

static void dwm1001_task_handler(void *arg){
	uint32_t * spidevp = (uint32_t *)arg;
	uint8_t spidev = (uint8_t)*spidevp;

	uint8_t len;

	int ret = RV_ERR;
	uint8_t off_counter = 0;

	memset(&dmw1001_status[spidev],0,sizeof(struct dmw1001_status_t));

	#ifdef HWV1_2
	dwm1001_hw_rst(spidev);
	#endif
	dwm_init(&spidev);
	#ifdef HWV1_0
	dwm_reset(&spidev);
	#endif
	dwm1001_param_init(spidev);
	vTaskDelay(pdMS_TO_TICKS(100));
	dwm_gpio_value_set(&spidev, DWM_GPIO_IDX_27, 0);

	for (;;) {

    	if(off_counter>=3){//Após três tentativas com erro, relata estado off e restarta
			if(status_event_group_dwm != NULL)
    			xEventGroupSetBits(status_event_group_dwm, STATUS_UWB_COMMUNICATION_FAIL_BIT);
    		off_counter = 0;
    		if((status & 0x20)!=0 && (uint16_t)dmw1001_status[spidev].node_id!=0x0000) dwm1001_send_alive_off((uint16_t)dmw1001_status[spidev].node_id);
    		ESP_LOGW(TAG,"DWM1001(%d) reseting...",spidev);
			#ifdef HWV1_2
			dwm1001_hw_rst(spidev);
			#endif
			#ifdef HWV1_0
			dwm_reset(&spidev);
			#endif
			dwm1001_param_init(spidev);
			vTaskDelay(pdMS_TO_TICKS(100));
    	}
    	dwm_gpio_cfg_output(&spidev, DWM_GPIO_IDX_27, 0);
    	ret = dwm_label_read(&spidev, dmw1001_status[spidev].label, &len);
    	if(ret != RV_OK) {
    		off_counter++;
    		ESP_LOGE(TAG,"DWM1001(%d) label_read error",spidev);
    		continue;
    	}

    	ret = dwm_cfg_get(&spidev,&dmw1001_status[spidev].cfg_node);
    	if(ret != RV_OK) {
			off_counter++;
			ESP_LOGE(TAG,"DWM1001(%d) cfg_get error",spidev);
			continue;
		}
    	ret = dwm_panid_get(&spidev,&dmw1001_status[spidev].panid);
    	if(ret != RV_OK) {
			off_counter++;
			ESP_LOGE(TAG,"DWM1001(%d) panid_get error",spidev);
			continue;
		}
    	ret = dwm_pos_get(&spidev, &dmw1001_status[spidev].dev_pos);
    	if(ret != RV_OK) {
			off_counter++;
			ESP_LOGE(TAG,"DWM1001(%d) pos_get error",spidev);
			continue;
		}
    	ret = dwm_node_id_get(&spidev, &dmw1001_status[spidev].node_id);
    	if(ret != RV_OK) {
			off_counter++;
			ESP_LOGE(TAG,"DWM1001(%d) id_get error",spidev);
			continue;
		}
    	off_counter = 0; // Se chegou até aqui, não houve nenhum erro.
		if(status_event_group_dwm != NULL)
    		xEventGroupSetBits(status_event_group_dwm, STATUS_UWB_COMMUNICATION_OK_BIT);
    	dwm_gpio_value_set(&spidev, DWM_GPIO_IDX_27, 1);
    	if((status & 0x20)!=0) dwm1001_send_alive_on(&dmw1001_status[spidev], spidev);

    	vTaskDelay(DWM1001_INTERVAL_CHECK / portTICK_PERIOD_MS);
	}
    vTaskDelete(NULL);
}

#include "lmh_spirx_drdy.h"
#include "hal_gpio.h"
void dwm1001_init(void){

	#ifdef DWM1001_MAIN_DEBUG
	ESP_LOGI(TAG,"Initializing DWM1001...");
	#endif
	gpio_config_t io_conf;
	status_event_group_dwm = getStatusEventGroup();
	
	//interrupt of rising edge
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = 1ULL<<AURA1_RST_PIN | 1ULL<<AURA2_RST_PIN;
	//set as input mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//enable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);

	dwm_spi_init();

	//RELAY TEST
	//interrupt of rising edge
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = 1ULL<<RELAY_PIN;
	//set as input mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//enable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);

	//gpio_set_level(RELAY_PIN,1);

	memset(&dwm1001_addr,0xFFFF,NUM_OF_DWM1001_MODULES*sizeof(uint16_t));


	#ifdef DWM1001_MODULE_0
	uint32_t spidev0 = DWM1001_MODULE_0;
	xTaskCreate(dwm1001_task_handler, "dwm1001_task0", 4 * 1024, (void*)&spidev0,  configMAX_PRIORITIES-10, NULL);
	#endif

	#ifdef DWM1001_MODULE_1
	vTaskDelay(100 / portTICK_PERIOD_MS);
	uint32_t spidev1 = DWM1001_MODULE_1;
	xTaskCreate(dwm1001_task_handler, "dwm1001_task1", 4 * 1024, (void*)&spidev1,  configMAX_PRIORITIES-11, NULL);
	#endif
}
