#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../button/button_headers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "config.h"
#include "status.h"
#include "freertos/timers.h"
#include "lmh_spirx_drdy.h"
#include "hal/gpio_types.h"

#define GPIO_INPUT_PIN_SEL  1ULL<<PAIRING_RESET_BUTTON
#define ESP_INTR_FLAG_DEFAULT 0

#define TIME_TO_PAIRING 5000
#define TIME_TO_RESET 10000

#define BUTTON_RELEASED		0
#define BUTTON_PAIRING	1
#define BUTTON_RESET	2

static uint8_t button_state = BUTTON_RELEASED;

static QueueHandle_t gpio_evt_queue = NULL;

EventGroupHandle_t status_event_group_button;

EventBits_t last_status_button;

static TimerHandle_t tmr_button;

extern uint8_t status;

TaskHandle_t commButtonTask_handler;

static void button_timer_cb ( TimerHandle_t xTimer ){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BUTTON_INFO("\nbutton_timer_cb!!!\n");
	xTimerStop( tmr_button, 10 );
	if(button_state == BUTTON_RELEASED) {
		button_state = BUTTON_PAIRING;
		BUTTON_INFO("BUTTON IDLE -> BUTTON PAIRING...\n");
		if(status_event_group_button != NULL)
			xEventGroupSetBitsFromISR(status_event_group_button, STATUS_BUTTON_PRESSED_PAIRING_BIT | STATUS_BUTTON_RELEASED_PRE_PAIRING_BIT, &xHigherPriorityTaskWoken);
		tmr_button = xTimerCreate("ButtonTimer", pdMS_TO_TICKS(TIME_TO_RESET), pdFALSE, ( void * )5, &button_timer_cb);
		if( xTimerStart( tmr_button, 10 ) != pdPASS ) {
			BUTTON_ERROR("Timer start error (RESET)\n");
		}
	}
	else if(button_state == BUTTON_PAIRING) {
		button_state = BUTTON_RESET;
		BUTTON_INFO("BUTTON PAIRING -> BUTTON RESET...\n");
		if(status_event_group_button != NULL)
			xEventGroupSetBitsFromISR(status_event_group_button, STATUS_BUTTON_PRESSED_RESET_BIT | STATUS_BUTTON_RELEASED_PAIRING_BIT, &xHigherPriorityTaskWoken);
	}
}


void IRAM_ATTR gpio_isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);

}

static uint8_t button_level_now = 0;
static uint8_t button_level_bef = 1;

static uint8_t aura1_level_now = 1;
static uint8_t aura1_level_bef = 0;

static uint8_t aura2_level_now = 1;
static uint8_t aura2_level_bef = 0;

static void button_task(void* arg){
    uint32_t io_num;
	int id = 5;
	button_level_now = gpio_get_level(PAIRING_RESET_BUTTON);
	aura1_level_now = gpio_get_level(AURA1_READY_PIN);
	aura2_level_now = gpio_get_level(AURA2_READY_PIN);
	uint8_t aura1 = 0;
	uint8_t aura2 = 1;

    for(;;) {
    	//BUTTON_INFO("BUTTON TASK");
    	if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
        	//BUTTON_INFO("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            if(io_num==PAIRING_RESET_BUTTON){
				button_level_now = gpio_get_level(io_num);
				if(button_level_bef != button_level_now)
				{
					if(button_level_now==0){ //borda de descida
						button_state = BUTTON_RELEASED;
						//Inicia timer:
						BUTTON_INFO("PRESS STARTED!");
						tmr_button = xTimerCreate("ButtonTimer", pdMS_TO_TICKS(TIME_TO_PAIRING), pdFALSE, ( void * )id, &button_timer_cb);
						if( xTimerStart(tmr_button, 10 ) != pdPASS ) {
							BUTTON_ERROR("Timer start error\n");
						}
						//Led piscar "apertando"
						if(status_event_group_button != NULL)
							xEventGroupSetBits(status_event_group_button, STATUS_BUTTON_PRESSED_PRE_PAIRING_BIT);
					}
					else if(button_level_now==1) //borda de subida
					{
						if(tmr_button!=NULL)xTimerStop( tmr_button, 10 );
						if(button_state == BUTTON_PAIRING){
							BUTTON_INFO("PAIRING MODE...");
							if(status_event_group_button != NULL)
								xEventGroupSetBits(status_event_group_button, STATUS_BUTTON_RELEASED_PAIRING_BIT | STATUS_PAIRING_INIT_BIT);
							reparing_wifi();
						}
						else if(button_state == BUTTON_RESET){
							BUTTON_INFO("RESET MODE...");
							if(status_event_group_button != NULL)
								xEventGroupSetBits(status_event_group_button, STATUS_RESET_BIT | STATUS_BUTTON_RELEASED_RESET_BIT);
							factory_reset();
							vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 SEG
							esp_restart();
						}
						else if(button_state == BUTTON_RELEASED) {
							BUTTON_INFO("PRESS CANCELED!\n");
							xEventGroupSetBits(status_event_group_button, last_status_button | STATUS_BUTTON_RELEASED_PRE_PAIRING_BIT | STATUS_BUTTON_RELEASED_PAIRING_BIT |  STATUS_BUTTON_RELEASED_RESET_BIT);
						}
					}
					button_level_bef = button_level_now;
				}
            }
            else if(io_num==AURA1_READY_PIN){
            	aura1_level_now = gpio_get_level(io_num);
				if(aura1_level_bef != aura1_level_now)
				{
					LMH_SPIRX_DRDY_DrdyCb(&aura1);
				}
				aura1_level_bef = aura1_level_now;
            }
            else if(io_num==AURA2_READY_PIN){
				aura2_level_now = gpio_get_level(io_num);
				if(aura2_level_bef != aura2_level_now)
				{
					LMH_SPIRX_DRDY_DrdyCb(&aura2);
				}
				aura2_level_bef = aura2_level_now;
			}
        }
    }
}

void button_init(void){
    gpio_config_t io_conf;
	status_event_group_button = getStatusEventGroup();
	last_status_button = getLastStatusEventBits();

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //start gpio task
    xTaskCreatePinnedToCore(button_task, "button_task", (1024*5), NULL, 0, &commButtonTask_handler, 0);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PAIRING_RESET_BUTTON, gpio_isr_handler, (void*) PAIRING_RESET_BUTTON);
}

