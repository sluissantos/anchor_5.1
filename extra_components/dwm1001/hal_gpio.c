/*! ------------------------------------------------------------------------------------------------------------------
 * @file    hal_gpio.c
 * @brief   utility to operate GPIOs
 *          this lib is Raspberry-Pi dependent
 *
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */
#include "button_headers.h"
#include "hal.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "config.h"

#define TAG "HAL_GPIO"

/** 
 * @brief initializes the GPIO utilities
 *
 * @param none
 *
 * @return Error code
 */
int HAL_GPIO_Init(void) {
//   if (wiringPiSetup () < 0)
//   {
//      ESP_LOGE(TAG,"Unable to Init gpio...") ;
//      return HAL_ERR;
//   }

	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = AURA1_READY_PIN_MASK;
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpioConfig.intr_type = GPIO_INTR_POSEDGE;

	gpio_config(&gpioConfig);

	return HAL_OK;
}

/** 
 * @brief setup pin interrupt callback function on certain condition
 *
 * @param [in] pin: GPIO pin number of the Raspberry Pi external connector.
 * @param [in] edge_type: GPIO edge type to trigger interrupt
 * @param [in] cb: callback function pointer to be called when interrupt on the pin happens. 
 *
 * @return Error code
 */
int HAL_GPIO_SetupCb(int pin, int edge_type, void (*cb)(void)) {
	//HAL_GPIO_Init();
//   if (wiringPiISR (pin, edge_type, cb) < 0)
//   {
//      ESP_LOGE(TAG,"Unable to setup GPIO ISR...") ;
//      return HAL_ERR;
//   }

	esp_err_t ret;
	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = 1ULL << pin;
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpioConfig.intr_type = edge_type;

	ret = gpio_config(&gpioConfig);
	if(ret != ESP_OK) return HAL_ERR;

	ret=gpio_isr_handler_add(pin, gpio_isr_handler, (void*) pin);
	//ret=gpio_isr_handler_add(pin, (void*) cb, NULL);
	if(ret != ESP_OK) return HAL_ERR;


	return HAL_OK;
}

/** 
 * @brief Reads the GPIO pin
 *
 * @param [in] pin: GPIO pin to be read.
 *
 * @return GPIO value
 */
int HAL_GPIO_PinRead(int pin) {
	//return digitalRead(pin);
	return gpio_get_level(pin);
}

