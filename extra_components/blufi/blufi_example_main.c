/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


/****************************************************************************
* This is a demo for bluetooth config wifi connection to ap. You can config ESP32 to connect a softap
* or config ESP32 as a softap to be connected by other device. APP can be downloaded from github
* android source code: https://github.com/EspressifApp/EspBlufi
* iOS source code: https://github.com/EspressifApp/EspBlufiForiOS
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_netif_types.h"
#include "esp_netif_ip_addr.h"
#include "storage_headers.h"
#include "status.h"
#include "config.h"


#include "esp_blufi_api.h"
#include "blufi_example.h"

#include "esp_blufi.h"

#define EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY CONFIG_EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY
#define EXAMPLE_INVALID_REASON                255
#define EXAMPLE_INVALID_RSSI                  -128

EventGroupHandle_t bluetooth_deinit_event_group_blufi;
uint16_t status_blufi;

static uint8_t blufi_running = 0;
static char device_name[32];
static uint8_t blue_product_info[10];

static void blue_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

static esp_ble_adv_data_t blue_adv_data = { .set_scan_rsp = false,
		.include_name = true,
		.include_txpower = false,
		.min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
		.max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
		.appearance = 0x00,
		.manufacturer_len = 9, .p_manufacturer_data = blue_product_info,
		.service_data_len = 0, .p_service_data = NULL,
		.flag = 0x6,
};

static esp_ble_adv_params_t blue_adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


#define WIFI_LIST_NUM   10
static const char *TAG = "WIFI";

static wifi_config_t sta_config;
static wifi_config_t ap_config;
wifi_data_base_t wifi_data;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static uint8_t example_wifi_retry = 0;

/* store the station info for send back to phone */
static bool gl_sta_connected = false;
static bool gl_sta_got_ip = false;
static bool ble_is_connected = false;
static uint8_t gl_sta_bssid[6];
static uint8_t gl_sta_ssid[32];
static int gl_sta_ssid_len;
static wifi_sta_list_t gl_sta_list;
static bool gl_sta_is_connecting = false;
static esp_blufi_extra_info_t gl_sta_conn_info;

/* connect infor*/
static uint8_t server_if;
static uint16_t conn_id;

struct nvs_data_t nvs_data_blufi;
struct factory_config_t factory_config_blufi;

static void example_record_wifi_conn_info(int rssi, uint8_t reason){
    memset(&gl_sta_conn_info, 0, sizeof(esp_blufi_extra_info_t));
    if (gl_sta_is_connecting) {
        gl_sta_conn_info.sta_max_conn_retry_set = true;
        gl_sta_conn_info.sta_max_conn_retry = EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY;
    } else {
        gl_sta_conn_info.sta_conn_rssi_set = true;
        gl_sta_conn_info.sta_conn_rssi = rssi;
        gl_sta_conn_info.sta_conn_end_reason_set = true;
        gl_sta_conn_info.sta_conn_end_reason = reason;
    }
}

static void example_wifi_connect(void){
    example_wifi_retry = 0;
    gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
    example_record_wifi_conn_info(EXAMPLE_INVALID_RSSI, EXAMPLE_INVALID_REASON);
}

static bool example_wifi_reconnect(void){
    bool ret;
    if (gl_sta_is_connecting && example_wifi_retry++ < EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY) {
        BLUFI_INFO("BLUFI WiFi starts reconnection\n");
        gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
        example_record_wifi_conn_info(EXAMPLE_INVALID_RSSI, EXAMPLE_INVALID_REASON);
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}

static int softap_get_current_connection_number(void){
    esp_err_t ret;
    ret = esp_wifi_ap_get_sta_list(&gl_sta_list);
    if (ret == ESP_OK)
    {
        return gl_sta_list.num;
    }

    return 0;
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data){

    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));

    wifi_mode_t mode;

    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            BLUFI_INFO("SET STATUS_WIFI_GOT_IP_BIT");
            xEventGroupSetBits(getStatusEventGroup(), STATUS_WIFI_GOT_IP_BIT);
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            if(blufi_running==1) {
                if ((status_blufi & 0x04) !=0 ){ //STATUS PAIRING BLE CONNECTED
                    esp_blufi_extra_info_t info;
                    esp_wifi_get_mode(&mode);
                    memset(&info, 0, sizeof(esp_blufi_extra_info_t));
                    memcpy(info.sta_bssid, gl_sta_bssid, 6);
                    info.sta_bssid_set = true;
                    info.sta_ssid = gl_sta_ssid;
                    info.sta_ssid_len = gl_sta_ssid_len;
                    gl_sta_got_ip = true;
                    BLUFI_INFO("ESP_BLUFI_STA_CONN_SUCCESS");
                    if (ble_is_connected == true) {
                        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, softap_get_current_connection_number(), &info);
                    } else {
                        BLUFI_INFO("BLUFI BLE is not connected yet\n");
                    }
                }
                wifi_config_t wifi_cfg;
                esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg);
                if(strcmp((const char*) wifi_cfg.sta.ssid,DEFAULT_SSID)==0 && strcmp((const char*) wifi_cfg.sta.password,DEFAULT_PASSWORD)==0){
                    blufi_security_deinit();
                    esp_ble_gap_stop_advertising();
                    esp_blufi_profile_deinit();
                }
            }
        }
        default:
            break;
    }
    return;
}

#define MONITOR_WATCHDOG_CONNECTION_WIFI 30 //x2 seconds
#define MONITOR_WATCHDOG_REBOOT 10
static uint8_t check_connect_times=0;
static uint8_t check_reboot_times=0;

int BLUETOOTH_DEINIT_BIT_BLUFI;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data){
    wifi_event_sta_connected_t *event;
    wifi_event_sta_disconnected_t *disconnected_event;
    wifi_mode_t mode;

    switch (event_id) {
        case WIFI_EVENT_STA_START:
            BLUFI_INFO("WIFI_EVENT_STA_START\n");
            example_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
            BLUFI_INFO("WIFI_EVENT_STA_CONNECTED\n");
            gl_sta_connected = true;
            gl_sta_is_connecting = false;
            event = (wifi_event_sta_connected_t*) event_data;
            memcpy(gl_sta_bssid, event->bssid, 6);
            memcpy(gl_sta_ssid, event->ssid, event->ssid_len);
            gl_sta_ssid_len = event->ssid_len;
            xEventGroupSetBits(getStatusEventGroup(), STATUS_WIFI_CONNECTED_BIT);
            check_connect_times=0;
            check_reboot_times=0;
            //config_hostname();
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED:
            BLUFI_INFO("WIFI_EVENT_STA_DISCONNECTED\n");
            /* Only handle reconnection during connecting */
            if (gl_sta_connected == false && example_wifi_reconnect() == false) {
                gl_sta_is_connecting = false;
                disconnected_event = (wifi_event_sta_disconnected_t*) event_data;
                example_record_wifi_conn_info(disconnected_event->rssi, disconnected_event->reason);
            }
            /* This is a workaround as ESP32 WiFi libs don't currently
            auto-reassociate. */
            gl_sta_connected = false;
            gl_sta_got_ip = false;
            memset(gl_sta_ssid, 0, 32);
            memset(gl_sta_bssid, 0, 6);
            gl_sta_ssid_len = 0;
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            if(check_connect_times>MONITOR_WATCHDOG_CONNECTION_WIFI) esp_wifi_stop();
            else check_connect_times++;
            break;

        case WIFI_EVENT_AP_START:
            BLUFI_INFO("WIFI_EVENT_AP_START\n");
            esp_wifi_get_mode(&mode);

            /* TODO: get config or information of softap, then set to report extra_info */
            if (ble_is_connected == true) {
                if (gl_sta_connected) {
                    esp_blufi_extra_info_t info;
                    memset(&info, 0, sizeof(esp_blufi_extra_info_t));
                    memcpy(info.sta_bssid, gl_sta_bssid, 6);
                    info.sta_bssid_set = true;
                    info.sta_ssid = gl_sta_ssid;
                    info.sta_ssid_len = gl_sta_ssid_len;
                    esp_blufi_send_wifi_conn_report(mode, gl_sta_got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
                } else if (gl_sta_is_connecting) {
                    if(blufi_running){
                        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &gl_sta_conn_info);
                    } else {
                        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &gl_sta_conn_info);
                    }
                }
            } else {
                BLUFI_INFO("BLUFI BLE is not connected yet\n");
            }
            break;

        case WIFI_EVENT_STA_STOP:
            BLUFI_INFO("WIFI_EVENT_STA_STOP\n");
            if(check_reboot_times>MONITOR_WATCHDOG_REBOOT) esp_restart();
            else check_reboot_times++;
            check_connect_times=0;
            example_wifi_connect();
            break;

        case WIFI_EVENT_SCAN_DONE: {
            BLUFI_INFO("WIFI_EVENT_SCAN_DONE\n");
            uint16_t apCount = 0;
            esp_wifi_scan_get_ap_num(&apCount);
            if (apCount == 0) {
                BLUFI_INFO("Nothing AP found");
                break;
            }
            wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
            if (!ap_list) {
                BLUFI_ERROR("malloc error, ap_list is NULL");
                break;
            }
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
            esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
            if (!blufi_ap_list) {
                if (ap_list) {
                    free(ap_list);
                }
                BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
                break;
            }
            for (int i = 0; i < apCount; ++i)
            {
                blufi_ap_list[i].rssi = ap_list[i].rssi;
                memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
            }

            if (ble_is_connected == true) {
                esp_blufi_send_wifi_list(apCount, blufi_ap_list);
            } else {
                BLUFI_INFO("BLUFI BLE is not connected yet\n");
            }

            esp_wifi_scan_stop();
            free(ap_list);
            free(blufi_ap_list);
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            BLUFI_INFO("station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            BLUFI_INFO("station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }

        default:
            BLUFI_INFO("WIFI: %ld\n",event_id);
            break;
    }
    return;
}

void initialise_wifi(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    storageGetString(STORAGE_OR_FACTORY_WIFI_SSID, wifi_data.ssid);
    storageGetString(STORAGE_OR_FACTORY_WIFI_PWD, wifi_data.pwd);
    storageGetUint8(STORAGE_OR_FACTORY_WIFI_AUTH_TYPE, &wifi_data.auth_type);

    //it's necessery start the ssid and the password like that. Reason = ?  Works = yes :D
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_OPEN
        }
    };

    strcpy((char *)wifi_config.sta.ssid, wifi_data.ssid);
    strcpy((char *)wifi_config.sta.password, wifi_data.pwd);
    wifi_config.sta.threshold.authmode = (wifi_auth_mode_t)wifi_data.auth_type;

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}


static esp_blufi_callbacks_t blufi_callbacks = {
    .event_cb = blue_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

static void blue_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param){
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        BLUFI_INFO("BLUFI init finish\n");

        esp_blufi_adv_start();

        esp_ble_gap_set_device_name(device_name);
        esp_ble_gap_config_adv_data(&blue_adv_data);
        //Pareamento Bluetooth inicializou, porém já havia conectado no WiFi, então, retroceder o pareamento Bluetooth.
		if ((status_blufi & 0x10) !=0 ){ //STATUS WIFI GOT IP
			blufi_security_deinit();
			esp_ble_gap_stop_advertising();
			esp_blufi_profile_deinit();
		}
		xEventGroupSetBits(getStatusEventGroup(), STATUS_PAIRING_INIT_BIT);
        break;

    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        BLUFI_INFO("BLUFI deinit finish\n");
        blufi_running = 0;
		xEventGroupSetBits(getStatusEventGroup(), STATUS_NO_PAIRING_BIT);
		if (gl_sta_connected) {
			xEventGroupSetBits(bluetooth_deinit_event_group_blufi, BLUETOOTH_DEINIT_BIT_BLUFI);
		}
        break;

    case ESP_BLUFI_EVENT_BLE_CONNECT:
        BLUFI_INFO("BLUFI ble connect\n");
        ble_is_connected = true;
        server_if = param->connect.server_if;
        conn_id = param->connect.conn_id;
        esp_blufi_adv_stop();
        blufi_security_init();
        xEventGroupSetBits(getStatusEventGroup(), STATUS_PAIRING_BLE_CONNECTED_BIT);
        break;

    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        BLUFI_INFO("BLUFI ble disconnect\n");
        ble_is_connected = false;
        blufi_security_deinit();
        xEventGroupSetBits(getStatusEventGroup(), STATUS_PAIRING_BLE_DISCONNECTED_BIT);
        if (gl_sta_connected){
            esp_blufi_adv_stop();
            esp_blufi_profile_deinit();
        }
        else{
            esp_blufi_adv_start();
        }
        break;

    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        BLUFI_INFO("BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
        ESP_ERROR_CHECK( esp_wifi_set_mode(param->wifi_mode.op_mode) );
        break;

    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        BLUFI_INFO("BLUFI requset wifi connect to AP\n");
        /* there is no wifi callback when the device has already connected to this wifi
        so disconnect wifi before connection.
        */
        esp_wifi_disconnect();
        example_wifi_connect();
        break;

    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        BLUFI_INFO("BLUFI requset wifi disconnect from AP\n");
        esp_wifi_disconnect();
        break;

    case ESP_BLUFI_EVENT_REPORT_ERROR:
        BLUFI_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
        esp_blufi_send_error_info(param->report_error.state);
        break;

    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        wifi_mode_t mode;
        esp_blufi_extra_info_t info;

        esp_wifi_get_mode(&mode);

        if (gl_sta_connected) {
            memset(&info, 0, sizeof(esp_blufi_extra_info_t));
            memcpy(info.sta_bssid, gl_sta_bssid, 6);
            info.sta_bssid_set = true;
            info.sta_ssid = gl_sta_ssid;
            info.sta_ssid_len = gl_sta_ssid_len;
            esp_blufi_send_wifi_conn_report(mode, gl_sta_got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
        } else if (gl_sta_is_connecting) {
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &gl_sta_conn_info);
        } else {
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &gl_sta_conn_info);
        }
        BLUFI_INFO("BLUFI get wifi status from AP\n");
        break;

    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        BLUFI_INFO("blufi close a gatt connection");
        esp_blufi_disconnect();
        break;

    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;

	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
        sta_config.sta.bssid_set = 1;
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA BSSID %s\n", sta_config.sta.ssid);
        break;

	case ESP_BLUFI_EVENT_RECV_STA_SSID:
        strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA SSID %s\n", sta_config.sta.ssid);
        break;

	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA PASSWORD %s\n", sta_config.sta.password);
        break;

	case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        strncpy((char *)ap_config.ap.ssid, (char *)param->softap_ssid.ssid, param->softap_ssid.ssid_len);
        ap_config.ap.ssid[param->softap_ssid.ssid_len] = '\0';
        ap_config.ap.ssid_len = param->softap_ssid.ssid_len;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP SSID %s, ssid len %d\n", ap_config.ap.ssid, ap_config.ap.ssid_len);
        break;

	case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        strncpy((char *)ap_config.ap.password, (char *)param->softap_passwd.passwd, param->softap_passwd.passwd_len);
        ap_config.ap.password[param->softap_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP PASSWORD %s len = %d\n", ap_config.ap.password, param->softap_passwd.passwd_len);
        break;

	case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        if (param->softap_max_conn_num.max_conn_num > 4) {
            return;
        }
        ap_config.ap.max_connection = param->softap_max_conn_num.max_conn_num;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP MAX CONN NUM %d\n", ap_config.ap.max_connection);
        break;

	case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        if (param->softap_auth_mode.auth_mode >= WIFI_AUTH_MAX) {
            return;
        }
        ap_config.ap.authmode = param->softap_auth_mode.auth_mode;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP AUTH MODE %d\n", ap_config.ap.authmode);
        break;

	case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        if (param->softap_channel.channel > 13) {
            return;
        }
        ap_config.ap.channel = param->softap_channel.channel;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP CHANNEL %d\n", ap_config.ap.channel);
        break;

    case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
        wifi_scan_config_t scanConf = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false
        };
        esp_err_t ret = esp_wifi_scan_start(&scanConf, true);
        if (ret != ESP_OK) {
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        }
        break;

    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        //BLUFI_INFO("Recv Custom Data %" PRIu32 "\n", param->custom_data.data_len);
        //esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);
        //Mini Protocol
        #define MINIPROTOCOLV1 0X01
        #define TYPECONFIG 0X01
        #define SUBTYPETIMEZONE 0X01
		if(param->custom_data.data_len>3){
			if(param->custom_data.data[0]==MINIPROTOCOLV1 && param->custom_data.data[1]==TYPECONFIG && param->custom_data.data[2]==SUBTYPETIMEZONE){
				memset(nvs_data_blufi.timezone, 0, sizeof(nvs_data_blufi.timezone));
				strncpy((char *)&nvs_data_blufi.timezone, (const char *)param->custom_data.data+4, param->custom_data.data_len-4);
				BLUFI_INFO("Timezone received: \"%s\"",nvs_data_blufi.timezone);
				nvs_handle config_handle;
				nvs_open("nvs", NVS_READWRITE, &config_handle);
				nvs_set_str(config_handle, "timezone", (const char *)&nvs_data_blufi.timezone);
				nvs_commit(config_handle);
				nvs_close(config_handle);
				setenv("TZ", nvs_data_blufi.timezone, 1);
				tzset();
			}
		}
        break;
        
	case ESP_BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;;
	case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }
}

static void blue_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&blue_adv_params);
            break;
        default:
            break;
    }
}

void blufi_init(void){
    esp_err_t ret;
    status_blufi = getStatus();
    nvs_data_blufi = getNvsData();
    factory_config_blufi = getFactoryConfig();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        BLUFI_ERROR("%s initialize bt controller failed: %s\n", __func__, esp_err_to_name(ret));
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        BLUFI_ERROR("%s enable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_blufi_host_and_cb_init(&blufi_callbacks);
    if (ret) {
        BLUFI_ERROR("%s initialise failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    //BLUFI_INFO("BLUFI VERSION %04x\n", esp_blufi_get_version());
    //uint8_t *bd_addr = esp_bt_dev_get_address();
    //BLUFI_INFO("BD ADDR: %02X:%02X:%02X:%02X:%02X:%02X\n", bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    BLUFI_INFO("BD ADDR: "ESP_BD_ADDR_STR"\n", ESP_BD_ADDR_HEX(esp_bt_dev_get_address()));

    	//Bluetooth Device Name:
	uint8_t wifi_sta_mac_addr[6] = {0};
	esp_read_mac(wifi_sta_mac_addr, ESP_MAC_WIFI_STA);

	sprintf(device_name,""HOSTNAME_BLE_PREFIX"p%02X%02X",wifi_sta_mac_addr[4],wifi_sta_mac_addr[5]);

	blue_product_info[0] = (uint8_t)factory_config_blufi.id_plat;
	blue_product_info[1] = (uint8_t)factory_config_blufi.hw_major;
	blue_product_info[2] = (uint8_t)factory_config_blufi.hw_minor;
	blue_product_info[3] = (uint8_t)factory_config_blufi.hw_rev;
	blue_product_info[4] = (uint8_t)FIRMWARE_TYPE;

	blue_product_info[5] = (uint8_t)CONFIG_PRODUCT_ID;
	blue_product_info[6] = (uint8_t)CONFIG_FIRMWARE_VERSION_MAJOR;
	blue_product_info[7] = (uint8_t)CONFIG_FIRMWARE_VERSION_MINOR;
	blue_product_info[8] = (uint8_t)CONFIG_FIRMWARE_VERSION_REVISION;
	blufi_running = 1;
	ret = esp_ble_gap_register_callback(blue_gap_event_handler);
    if(ret){
        BLUFI_ERROR("%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_blufi_register_callbacks(&blufi_callbacks);
    if(ret){
        BLUFI_ERROR("%s blufi register failed, error code = %x\n", __func__, ret);
        return;
    }

    esp_blufi_profile_init();
}

EventGroupHandle_t getWifiEventGroup(){
    return wifi_event_group;
}

void setBlueEventGroup(EventGroupHandle_t event){
    bluetooth_deinit_event_group_blufi = event;
    return;
}

void setBlueDeinitBit(int BLUETOOTH_DEINIT_BIT){
    BLUETOOTH_DEINIT_BIT_BLUFI = BLUETOOTH_DEINIT_BIT;
    return;
}