#pragma once
#include "esp_system.h"
#include "freertos/event_groups.h"

#define STATUS_NO_RESET_BIT BIT0 //1 (1<<0)
#define STATUS_RESET_BIT BIT1 //2 (1<<1)
#define	STATUS_NO_PAIRING_BIT BIT2 //4
#define	STATUS_PAIRING_INIT_BIT BIT3 //8
#define	STATUS_PAIRING_BLE_DISCONNECTED_BIT BIT4  //16
#define	STATUS_PAIRING_BLE_CONNECTED_BIT BIT5 //32
#define STATUS_WIFI_DISCONNECTED_BIT BIT6 //64
#define STATUS_WIFI_CONNECTED_BIT BIT7 //128
#define STATUS_WIFI_GOT_IP_BIT BIT8 //256
#define STATUS_MQTT_DISCONNECTED_BIT BIT9 //512
#define STATUS_MQTT_CONNECTED_BIT BIT10 //1024
#define STATUS_FWUPDATE_BIT BIT11 //2048
#define STATUS_BLINK_LED_OFF_BIT BIT12 //4096
#define STATUS_BLINK_LED_ON_BIT BIT13 //8192
#define STATUS_BUTTON_PRESSED_PAIRING_BIT BIT14 //16384
#define STATUS_BUTTON_RELEASED_PAIRING_BIT BIT15 //32768
#define STATUS_BUTTON_PRESSED_RESET_BIT BIT16 //65536
#define STATUS_BUTTON_RELEASED_RESET_BIT BIT17 //131072
#define STATUS_BUTTON_PRESSED_PRE_PAIRING_BIT BIT18 //262.144
#define STATUS_BUTTON_RELEASED_PRE_PAIRING_BIT BIT19 //524.288
#define STATUS_UWB_COMMUNICATION_FAIL_BIT BIT20 //1.048.576
#define STATUS_UWB_COMMUNICATION_OK_BIT BIT21 //2.097.152

void status_event_group_init(void);

EventGroupHandle_t getStatusEventGroup(void);
EventBits_t getLastStatusEventBits(void);
uint8_t getLedColorBlink(void);
uint16_t getStatus(void);

enum STATUS {
	STATUS_RESET=0,
	STATUS_PAIRING,
	STATUS_PAIRING_BLE_CONNECTED,
	STATUS_WIFI_CONNECTED,
	STATUS_WIFI_GOT_IP,
	STATUS_MQTT_CONNECTED,
	STATUS_FWUPDATE,
	STATUS_BLINK_LED,
	STATUS_BUTTON_PRESSED_PAIRING,
	STATUS_BUTTON_PRESSED_RESET,
	STATUS_BUTTON_PRESSED_PRE_PAIRING,
	STATUS_UWB_COMMUNICATION
};

enum{
	LED_BLINK_RF,
	LED_BLINK_LOCAL_NETWORK,
	LED_BLINK_REMOTE_NETWORK
};
