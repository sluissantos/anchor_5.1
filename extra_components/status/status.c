#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#include "esp_log.h"
#include "status.h"
#include "ledc_headers.h"

#define TAG "STATUS"

TaskHandle_t commStatusTask_handler;

EventGroupHandle_t status_event_group;
EventBits_t last_status;
uint8_t led_color_blink;
/*
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
*/

uint32_t status=0;

void status_task(void * parm){
    EventBits_t uxBits;
    while (1) {
    	uxBits = xEventGroupWaitBits(status_event_group, 0x00ffffff , pdTRUE, pdFALSE, portMAX_DELAY);
    	//ESP_LOGI(TAG, "uxBits=%d",uxBits);
		if((uxBits & STATUS_RESET_BIT) != 0) {
			status |= (1<<STATUS_RESET);
		}
		if((uxBits & STATUS_PAIRING_BLE_CONNECTED_BIT) == STATUS_PAIRING_BLE_CONNECTED_BIT){
			status |= (1<<STATUS_PAIRING_BLE_CONNECTED);
		}
		if((uxBits & STATUS_PAIRING_BLE_DISCONNECTED_BIT) == STATUS_PAIRING_BLE_DISCONNECTED_BIT){
			status &= ~(1<<STATUS_PAIRING_BLE_CONNECTED);
		}
		if((uxBits & STATUS_NO_PAIRING_BIT) != 0) {
			status &= ~(1<<STATUS_PAIRING);
		}
		if((uxBits & STATUS_PAIRING_INIT_BIT) != 0) {
			status |= (1<<STATUS_PAIRING);
		}
		if((uxBits & STATUS_WIFI_DISCONNECTED_BIT) != 0) {
			status &= ~(1<<STATUS_WIFI_CONNECTED);
			status &= ~(1<<STATUS_WIFI_GOT_IP);
		}
		if((uxBits & STATUS_FWUPDATE_BIT) == STATUS_FWUPDATE_BIT){
			status |= (1<<STATUS_FWUPDATE);
		}
		if((uxBits & STATUS_MQTT_DISCONNECTED_BIT) == STATUS_MQTT_DISCONNECTED_BIT){
			status &= ~(1<<STATUS_MQTT_CONNECTED);
		}
		if((uxBits & STATUS_MQTT_CONNECTED_BIT) == STATUS_MQTT_CONNECTED_BIT){
			status |= (1<<STATUS_MQTT_CONNECTED);
		}
		if((uxBits & STATUS_WIFI_GOT_IP_BIT) == STATUS_WIFI_GOT_IP_BIT){
			status |= (1<<STATUS_WIFI_GOT_IP);
		}
		if((uxBits & STATUS_WIFI_CONNECTED_BIT) != 0) {
			status |= (1<<STATUS_WIFI_CONNECTED);
		}
		if((uxBits & STATUS_NO_RESET_BIT) != 0) {
			status &= ~(1<<STATUS_RESET);
		}
		if((uxBits & STATUS_BLINK_LED_OFF_BIT) == STATUS_BLINK_LED_OFF_BIT){
			status &= ~(1<<STATUS_BLINK_LED);
		}
		if((uxBits & STATUS_BLINK_LED_ON_BIT) == STATUS_BLINK_LED_ON_BIT){
			status |= (1<<STATUS_BLINK_LED);
		}
		if((uxBits & STATUS_BUTTON_PRESSED_PAIRING_BIT) == STATUS_BUTTON_PRESSED_PAIRING_BIT){
			status |= (1<<STATUS_BUTTON_PRESSED_PAIRING);
		}
		if((uxBits & STATUS_BUTTON_RELEASED_PAIRING_BIT) == STATUS_BUTTON_RELEASED_PAIRING_BIT){
			status &= ~(1<<STATUS_BUTTON_PRESSED_PAIRING);
		}
		if((uxBits & STATUS_BUTTON_PRESSED_RESET_BIT) == STATUS_BUTTON_PRESSED_RESET_BIT){
			status |= (1<<STATUS_BUTTON_PRESSED_RESET);
		}
		if((uxBits & STATUS_BUTTON_RELEASED_RESET_BIT) == STATUS_BUTTON_RELEASED_RESET_BIT){
			status &= ~(1<<STATUS_BUTTON_PRESSED_RESET);
		}
		if((uxBits & STATUS_BUTTON_PRESSED_PRE_PAIRING_BIT) == STATUS_BUTTON_PRESSED_PRE_PAIRING_BIT){
			status |= (1<<STATUS_BUTTON_PRESSED_PRE_PAIRING);
		}
		if((uxBits & STATUS_BUTTON_RELEASED_PRE_PAIRING_BIT) == STATUS_BUTTON_RELEASED_PRE_PAIRING_BIT){
			status &= ~(1<<STATUS_BUTTON_PRESSED_PRE_PAIRING);
		}
		if((uxBits & STATUS_UWB_COMMUNICATION_FAIL_BIT) != 0) {
			status |= (1<<STATUS_UWB_COMMUNICATION);
		}
		if((uxBits & STATUS_UWB_COMMUNICATION_OK_BIT) != 0) {
			status &= ~(1<<STATUS_UWB_COMMUNICATION);
		}

		last_status = uxBits;

//		ESP_LOGI(TAG,"Status: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(status));
//		if ((status & 0x01) !=0 ) ESP_LOGI(TAG, "STATUS RESET");
//		if ((status & 0x02) !=0 ) ESP_LOGI(TAG, "STATUS PAIRING");
//		if ((status & 0x04) !=0 ) ESP_LOGI(TAG, "STATUS PAIRING BLE CONNECTED");
//		if ((status & 0x08) !=0 ) ESP_LOGI(TAG, "STATUS WIFI CONNECTED");
//		if ((status & 0x10) !=0 ) ESP_LOGI(TAG, "STATUS WIFI GOT IP");
//		if ((status & 0x20) !=0 ) ESP_LOGI(TAG, "STATUS MQTT CONNECTED");
//		if ((status & 0x40) !=0 ) ESP_LOGI(TAG, "STATUS FWUPDATE");
//		if ((status & 0x80) !=0 ) ESP_LOGI(TAG, "STATUS BIT8");

		//LED STATUS:
		if ((status & 0x400) !=0 ) led_handler(LED_BUTTON_PRESSED_PRE_PAIRING);
		else if ((status & 0x100) !=0 ) led_handler(LED_BUTTON_PRESSED_PAIRING);
		else if ((status & 0x200) !=0 ) led_handler(LED_BUTTON_PRESSED_RESET);
		else if ((status & 0x01) !=0 ) led_handler(LED_RESET_ACTIVED);
		else if ((status & 0x40) !=0 ) led_handler(LED_UPDATING);
		else if ((status & 0x04) !=0 ) led_handler(LED_PAIRING_CONNECTED);
		else if ((status & 0x02) !=0 ) led_handler(LED_PAIRING_ACTIVED);
		else if ((status & 0x800) !=0 ) led_handler(LED_UWB_COMMUNICATION_FAIL);
		else if ((status & 0x80) !=0 ) led_handler(LED_DATA_BLINK);
		else if ((status & 0x38) == 0 ) led_handler(LED_WIFI_DISCONNECTED);
		else if ((status & 0x20) !=0 ) led_handler(LED_WIFI_CLOUD_CONNECTED);
		else if ((status & 0x10) !=0 ) led_handler(LED_WIFI_GOT_IP);
		else if ((status & 0x08) !=0 ) led_handler(LED_WIFI_CONNECTED);
		taskYIELD();
    }
}

void status_event_group_init(void){
    status_event_group = xEventGroupCreate();

	if( status_event_group == NULL ){
		ESP_LOGE(TAG, "STATUS event group failed!");
	}
	else{
		xTaskCreatePinnedToCore(status_task, "status_task", (1024*2), NULL, 0, &commStatusTask_handler, 0);
		//ESP_LOGI(TAG, "ENTROU AQUI");
		//uxTaskGetStackHighWaterMark(NULL);
	}
}

EventGroupHandle_t getStatusEventGroup(){
	return status_event_group;
}

EventBits_t getLastStatusEventBits(){
	return last_status;
}

uint8_t getLedColorBlink(){
	return led_color_blink;
}

uint32_t getStatus(){
	return status;
}