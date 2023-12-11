/*! ------------------------------------------------------------------------------------------------------------------
 * @file    hal_gpio.h
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
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_
#include "driver/gpio.h"
#include "config.h"

#define HAL_GPIO_INT_EDGE_FALLING   GPIO_INTR_NEGEDGE
#define HAL_GPIO_INT_EDGE_RISING    GPIO_INTR_POSEDGE
#define HAL_GPIO_INT_EDGE_BOTH      GPIO_INTR_ANYEDGE
#define HAL_GPIO_INT_EDGE_SETUP     GPIO_INTR_DISABLE

#define HAL_GPIO_DRDY0  AURA1_READY_PIN
#define HAL_GPIO_DRDY1  AURA2_READY_PIN

/** 
 * @brief initializes the GPIO utilities
 *
 * @param none
 *
 * @return Error code
 */
int HAL_GPIO_Init(void);

/** 
 * @brief setup pin interrupt callback function on certain condition
 *
 * @param [in] pin: GPIO pin number of the Raspberry Pi external connector.
 * @param [in] edge_type: GPIO edge type to trigger interrupt
 * @param [in] cb: callback function pointer to be called when interrupt on the pin happens. 
 *
 * @return Error code
 */
int HAL_GPIO_SetupCb(int pin, int edge_type, void (*cb)(void));

/** 
 * @brief Reads the GPIO pin
 *
 * @param [in] pin: GPIO pin to be read.
 *
 * @return GPIO value
 */
int HAL_GPIO_PinRead(int pin);

#endif //_HAL_GPIO_H_


