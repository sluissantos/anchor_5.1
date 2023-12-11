#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "ledc_headers.h"
#include "esp_log.h"
#include "config.h"
#include "status.h"

#define LED_STACK_SIZE 3*configMINIMAL_STACK_SIZE
#define LED_PRIORITY_TASK 5

#define TAG "LEDC"

#define MODE      LEDC_LOW_SPEED_MODE
#define TIMER     LEDC_TIMER_1
#define DUTY_BITS LEDC_TIMER_13_BIT

#define LEDC_MAX_FADE_DUTY         (8191) //max 2*4096 (13 bit) -1
#define LEDC_FADE_TIME_SLOW    (2000) //4segundos de fade
#define LEDC_FADE_TIME_MEDIUM    (750)
#define LEDC_FADE_TIME_FAST    (500) //1 segundo de fade
#define LEDC_FADE_TIME_ULTRA_FAST    (50) //100ms de fade

#define LEDC_MAX_CH_NUM 2

#define TYPE_FADE 0
#define TYPE_BLINK 1
#define TYPE_DATA 2
#define TYPE_ON 3
#define TYPE_OFF 4

#define LED_R 0
#define LED_B 1

#define LED_ON 1
#define LED_OFF 0

#define LEDC_CPU_0 0
#define LEDC_CPU_1 1
#define LEDC_CPU_SELECT LEDC_CPU_1

struct led_cmd_t{
	uint8_t type;
	uint16_t time;
	bool initial_state;
	uint8_t ch;
	TaskHandle_t xHandle;
	SemaphoreHandle_t semaphore;
};

static struct led_cmd_t R_led_cmd;
static struct led_cmd_t B_led_cmd;

uint8_t led_color_blink_ledc;
EventGroupHandle_t status_event_group_ledc;

ledc_channel_config_t ledc[LEDC_MAX_CH_NUM];

void ledc_task( void * pvParameters ) {
	status_event_group_ledc = getStatusEventGroup();
	struct led_cmd_t led_cmd = *(struct led_cmd_t *)pvParameters;
	static bool fade_inverted;
	fade_inverted = led_cmd.initial_state;
	switch(led_cmd.type){
		case TYPE_FADE:
			for(;;) {
				xSemaphoreTake(led_cmd.semaphore, portMAX_DELAY);
				ledc_set_fade_with_time(MODE, ledc[led_cmd.ch].channel, fade_inverted ? 0 : LEDC_MAX_FADE_DUTY, led_cmd.time);
				ledc_fade_start(MODE, ledc[led_cmd.ch].channel, LEDC_FADE_WAIT_DONE);
				xSemaphoreGive(led_cmd.semaphore);
				fade_inverted = !fade_inverted;
			}
		break;
		case TYPE_BLINK:
			for(;;) {
				xSemaphoreTake(led_cmd.semaphore, portMAX_DELAY);
				ledc_set_duty(MODE, ledc[led_cmd.ch].channel,fade_inverted ? 0 : LEDC_MAX_FADE_DUTY);
				ledc_update_duty(MODE, ledc[led_cmd.ch].channel);
				xSemaphoreGive(led_cmd.semaphore);
				fade_inverted = !fade_inverted;
				vTaskDelay(led_cmd.time);
			}
		break;
		case TYPE_DATA:
			//printf("%d type DATA\n",led_cmd.ch);
			xSemaphoreTake(led_cmd.semaphore, portMAX_DELAY);
			ledc_set_duty(MODE, ledc[led_cmd.ch].channel,fade_inverted ? 0 : LEDC_MAX_FADE_DUTY);
			ledc_update_duty(MODE, ledc[led_cmd.ch].channel);
			xSemaphoreGive(led_cmd.semaphore);
			fade_inverted = !fade_inverted;
			vTaskDelay(led_cmd.time);

			xSemaphoreTake(led_cmd.semaphore, portMAX_DELAY);
			ledc_set_duty(MODE, ledc[led_cmd.ch].channel,fade_inverted ? 0 : LEDC_MAX_FADE_DUTY);
			ledc_update_duty(MODE, ledc[led_cmd.ch].channel);
			xSemaphoreGive(led_cmd.semaphore);
			fade_inverted = !fade_inverted;
			vTaskDelay(led_cmd.time);
			if(status_event_group_ledc != NULL)
				xEventGroupSetBits(status_event_group_ledc, STATUS_BLINK_LED_OFF_BIT);
		break;
	}
	xSemaphoreTake(led_cmd.semaphore, portMAX_DELAY);
	led_cmd.xHandle=NULL;
	xSemaphoreGive(led_cmd.semaphore);
	vTaskDelete(NULL);
}

static void led_off(uint8_t ch){
	ledc_set_duty(MODE, ledc[ch].channel, 0);
	ledc_update_duty(MODE, ledc[ch].channel);
}

static void led_on(uint8_t ch){
	ledc_set_duty(MODE, ledc[ch].channel, LEDC_MAX_FADE_DUTY);
	ledc_update_duty(MODE, ledc[ch].channel);
}

static void led_delete_xHandle(void){
	if(R_led_cmd.xHandle!=NULL) {
		xSemaphoreTake(R_led_cmd.semaphore, portMAX_DELAY);
		vTaskDelete(R_led_cmd.xHandle);
		xSemaphoreGive(R_led_cmd.semaphore);
		R_led_cmd.xHandle=NULL;
	}
	if(B_led_cmd.xHandle!=NULL) {
		xSemaphoreTake(B_led_cmd.semaphore, portMAX_DELAY);
		vTaskDelete(B_led_cmd.xHandle);
		xSemaphoreGive(B_led_cmd.semaphore);
		B_led_cmd.xHandle=NULL;
	}

	R_led_cmd.initial_state = LED_ON;
	B_led_cmd.initial_state = LED_ON;
}

void led_handler(uint8_t led_state){

	led_delete_xHandle();
	led_color_blink_ledc = getLedColorBlink();

	switch(led_state){
		case LED_WIFI_CONNECTED:
			ESP_LOGI(TAG,"LED_WIFI_CONNECTED");
			led_on(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_MEDIUM;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_WIFI_GOT_IP:
			ESP_LOGI(TAG,"LED_WIFI_GOT_IP");
			led_off(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_MEDIUM;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_WIFI_CLOUD_CONNECTED:
			ESP_LOGI(TAG,"LED_WIFI_CLOUD_CONNECTED");
			led_off(LED_R);
			B_led_cmd.type=TYPE_BLINK;
			B_led_cmd.time=LEDC_FADE_TIME_MEDIUM;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_WIFI_DISCONNECTED:
			ESP_LOGI(TAG,"LED_WIFI_DISCONNECTED");
			led_on(LED_R);
			led_on(LED_B);
			break;

		case LED_BUTTON_PRESSED_PRE_PAIRING:
			ESP_LOGI(TAG,"LED_BUTTON_PRESSED_PRE_PAIRING");
			led_off(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_FAST;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);

			break;

		case LED_BUTTON_PRESSED_PAIRING:
			ESP_LOGI(TAG,"LED_BUTTON_PRESSED_PAIRING");
			led_off(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_PAIRING_ACTIVED:
			ESP_LOGI(TAG,"LED_PAIRING_ACTIVED");
			led_off(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_SLOW;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_PAIRING_CONNECTED:
			ESP_LOGI(TAG,"LED_PAIRING_CONNECTED");
			led_on(LED_R);
			B_led_cmd.type=TYPE_FADE;
			B_led_cmd.time=LEDC_FADE_TIME_SLOW;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_BUTTON_PRESSED_RESET:
			ESP_LOGI(TAG,"LED_BUTTON_PRESSED_RESET");
			R_led_cmd.type=TYPE_FADE;
			R_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_R", LED_STACK_SIZE, &R_led_cmd, LED_PRIORITY_TASK, &R_led_cmd.xHandle, LEDC_CPU_SELECT);
			led_off(LED_B);
			break;

		case LED_RESET_ACTIVED:
			ESP_LOGI(TAG,"LED_RESET_ACTIVED");
			led_on(LED_R);
			led_off(LED_B);
			break;

		case LED_UPDATING:
			ESP_LOGI(TAG,"LED_UPDATING");
			led_on(LED_R);
			B_led_cmd.type=TYPE_BLINK;
			B_led_cmd.time=LEDC_FADE_TIME_FAST;
			B_led_cmd.initial_state=LED_OFF;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			break;

		case LED_UPDATE_SUCCESS:
			ESP_LOGI(TAG,"LED_UPDATE_SUCCESS");
			led_off(LED_R);
			B_led_cmd.type=TYPE_BLINK;
			B_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
			led_off(LED_B);
			break;

		case LED_UPDATE_FAIL:
			ESP_LOGI(TAG,"LED_UPDATE_FAIL");
			R_led_cmd.type=TYPE_BLINK;
			R_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
			xTaskCreatePinnedToCore(ledc_task, "ledc_task_R", LED_STACK_SIZE, &R_led_cmd, LED_PRIORITY_TASK, &R_led_cmd.xHandle, LEDC_CPU_SELECT);
			led_off(LED_B);
			break;

		case LED_UWB_COMMUNICATION_FAIL:
			ESP_LOGI(TAG,"LED_UWB_COMMUNICATION_FAIL");
			led_on(LED_R);
			led_off(LED_B);
		    break;

		case LED_DATA_BLINK:
			switch(led_color_blink_ledc){
				case LED_BLINK_RF:
					ESP_LOGI(TAG,"LED_DATA_BLINK_RF");
					led_off(LED_R);
					B_led_cmd.type=TYPE_DATA;
					B_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
					xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
					led_off(LED_B);
					break;
				case LED_BLINK_LOCAL_NETWORK:
					ESP_LOGI(TAG,"LED_DATA_BLINK_LOCAL");
					led_off(LED_R);
					B_led_cmd.type=TYPE_DATA;
					B_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
					xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
					break;
				case LED_BLINK_REMOTE_NETWORK:
					ESP_LOGI(TAG,"LED_DATA_BLINK_REMOTE");
					led_off(LED_R);
					B_led_cmd.type=TYPE_DATA;
					B_led_cmd.time=LEDC_FADE_TIME_ULTRA_FAST;
					xTaskCreatePinnedToCore(ledc_task, "ledc_task_B", LED_STACK_SIZE, &B_led_cmd, LED_PRIORITY_TASK, &B_led_cmd.xHandle, LEDC_CPU_SELECT);
					break;
			}
			break;
		default:
			ESP_LOGI(TAG,"LED STATE UNKNOWN");
			led_on(LED_R);
			led_on(LED_B);
			break;
	}
}

void led_init(void) {
	led_color_blink_ledc = getLedColorBlink();
	status_event_group_ledc = getStatusEventGroup();
	ledc_timer_config_t ledc_timer = {
		.duty_resolution = DUTY_BITS, // resolution of pwm duty
		.freq_hz = 5000,         // frequency of pwm signal
		.speed_mode = MODE,         // timer mode
		.timer_num = TIMER,         // timer index
		.clk_cfg = LEDC_AUTO_CLK,
	};

	ledc_timer_config(&ledc_timer);

	ledc[LED_R].channel = LEDC_CHANNEL_0;
	ledc[LED_R].duty = 0;
	ledc[LED_R].gpio_num = 25;
	ledc[LED_R].speed_mode = MODE;
	ledc[LED_R].hpoint = 0;
	ledc[LED_R].timer_sel = TIMER;

	ledc[LED_B].channel = LEDC_CHANNEL_1;
	ledc[LED_B].duty = 0;
	ledc[LED_B].gpio_num = 27;
	ledc[LED_B].speed_mode = MODE;
	ledc[LED_B].hpoint = 0;
	ledc[LED_B].timer_sel = TIMER;

	R_led_cmd.ch = LED_R;
	B_led_cmd.ch = LED_B;

	R_led_cmd.xHandle = NULL;
	B_led_cmd.xHandle = NULL;

	R_led_cmd.type = TYPE_OFF;
	B_led_cmd.type = TYPE_OFF;

	R_led_cmd.time = 0;
	B_led_cmd.type = 0;

	R_led_cmd.semaphore = xSemaphoreCreateBinary();
	B_led_cmd.semaphore = xSemaphoreCreateBinary();

	xSemaphoreGive(R_led_cmd.semaphore);
	xSemaphoreGive(B_led_cmd.semaphore);

	for (uint8_t ch = 0; ch < LEDC_MAX_CH_NUM; ch++) {
		ledc_channel_config(&ledc[ch]);
	}

	ESP_ERROR_CHECK(ledc_fade_func_install(0));
}
