#include <errno.h>
#include <stdio.h>
#include <string.h>

//#include "../components/button/button_headers.h"
//#include "../components/dwm1001/dwm1001_main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_err.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "blufi_example.h"
#include "config.h"
#include "ledc_headers.h"
#include "status.h"
#include "storage_headers.h"
//#include "mqtt_headers.h"
#include "https_ota.h"
#include "button_headers.h"
//#include "sntp_headers.h"
#include "dwm1001_main.h"
#include "esp_system.h"

#ifdef CONFIG_BOOTLOADER_WDT_DISABLE_IN_USER_CODE
#include "rtc_wdt.h"
#endif

#define TAG "AURA_ANCHOR_GW"

//#define TASKS_LIST_DEBUG
#ifdef TASKS_LIST_DEBUG
#define CONFIG_FREERTOS_USE_TRACE_FACILITY
#define CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
#endif
#ifdef TASKS_LIST_DEBUG
static int tasks_info(void){
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
        return 1;
    }
    fputs("Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
    fputs("\tAffinity", stdout);
    fputs("\n", stdout);
    vTaskList(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
    return 0;
}
#endif
int BLUETOOTH_DEINIT_BIT = BIT0;
int BLUETOOTH_MESH_STARTED_BIT = BIT1;

SemaphoreHandle_t init_mux;
EventGroupHandle_t bluetooth_deinit_event_group, status_event_main;
extern uint8_t status;
struct nvs_data_t nvs_data_main;


static esp_err_t bluetooth_init(void){
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    return ret;
}

static esp_err_t bluetooth_deinit(void){
    esp_err_t err;
    ESP_LOGI(TAG, "Free mem at start of bluetooth_deinit %ld", esp_get_free_heap_size());
    err = esp_bluedroid_disable();
    if (err != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_bluedroid_disable called successfully");
    err = esp_bluedroid_deinit();
    if (err != ESP_OK) {
        return err;
    }
    ESP_LOGI(TAG, "esp_bluedroid_deinit called successfully");
    err = esp_bt_controller_disable();
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "esp_bt_controller_disable called successfully");
    err = esp_bt_controller_deinit();
    if (err != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_bt_controller_deinit called successfully");

    esp_bt_mem_release(ESP_BT_MODE_BTDM);

    ESP_LOGI(TAG, "Free mem at end of bluetooth_deinit %ld", esp_get_free_heap_size());
    return ESP_OK;
}


void blufi_deinit_task(void * parm){
    EventBits_t uxBits;

    ESP_LOGI(TAG, "bluefi_deinit task initializing...");

    while (1) {
        uxBits = xEventGroupWaitBits(bluetooth_deinit_event_group, BLUETOOTH_DEINIT_BIT | BLUETOOTH_MESH_STARTED_BIT, true, false, portMAX_DELAY);
        if(uxBits & BLUETOOTH_DEINIT_BIT) {
            ESP_LOGI(TAG, "bluefi_deinit task");
            esp_err_t err = bluetooth_deinit();
			if (err) {
				ESP_LOGE(TAG, "Bluetooth stop (err %d)", err);
				return;
			}
			break;
        }
    }
    vTaskDelete(NULL);
}


void bluefi_deinit_task_init(void){
	bluetooth_deinit_event_group = xEventGroupCreate();
	xTaskCreate(blufi_deinit_task, "blufi_deinit_task", 1024, NULL, 5, NULL);
}

void app_main(void){
    ESP_LOGI(TAG, "Initializing...");
    initStorage();
	init_mux = xSemaphoreCreateMutex();
    led_init();
    status_event_group_init();
    status_event_main = getStatusEventGroup();
    xEventGroupSetBits(status_event_main, STATUS_NO_RESET_BIT);
    factory_init_data();
    config_base_mac();
    initialise_wifi();
    config_load();
    nvs_data_main = getNvsData();
	if(nvs_data_main.fw_update_data.attempts==0) {
		xSemaphoreTake(init_mux, portMAX_DELAY);
		button_init();
		//aura_board_init();
		//dwm1001_init();
		//sntp_main_init();
		//mqtt_init();
		//xSemaphoreGive(init_mux);
	} else {
		//xSemaphoreTake(init_mux, portMAX_DELAY);
		//ota_init(&nvs_data.fw_update_data);
		//xSemaphoreGive(init_mux);
	}
	

    //blufi_init();
}
