#pragma once
#include "esp_system.h"
#include "config.h"
#include "mqtt_client.h"

void mqtt_init(void);

#define MQTT_TOPIC_MANUFACTURER "lpx"
#define MQTT_TOPIC_PROJECT "szu"
#define MQTT_TOPIC_BROAD "broad"
#define MQTT_DOWNTOPIC "/down"
#define MQTT_UPTOPIC "/up"
#define MQTT_LWTTOPIC "/up/status"

#define MQTT_UWBTOPIC "/uwb"
#define MQTT_CMD_UPGRADE "/cmd/upgrade"
#define MQTT_CMD_REBOOT "/cmd/reboot"
#define MQTT_CMD_CONFIG_UWB "/cmd/uwb"
#define MQTT_CMD_CONFIG_WIFI "/cmd/wifi"
#define MQTT_CMD_FACTORY_RESET "/cmd/factreset"
#define LWT_MESSAGE_OFFLINE "{\"alive\":\"off\"}"
#define ONLINE_MESSAGE "{\"alive\":\"on\"}"

#define RETAIN_ON 1
#define RETAIN_OFF 0
#define QOS0 0
#define QOS1 1
#define QOS2 2

struct mqtt_broker_conf {
	char uri[128];
	char user[128];
	char pwd[128];
};

struct mqtt_conf_t {
	char client_id[64]; //The Server MUST allow ClientIds which are between 1 and 23 UTF-8 encoded bytes in length, and that contain only the characters "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" [MQTT-3.1.3-5].
	char lwt_topic[64];
	char up_topic[64];
	char down_topic[64];
	char broad_down_topic[64];
	//char uwb_topic[64];
};
