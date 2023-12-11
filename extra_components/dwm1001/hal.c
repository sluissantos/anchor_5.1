/*! ------------------------------------------------------------------------------------------------------------------
 * @file    hal.c
 * @brief   hardware abstraction layer (HAL) source file
 *
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */
 
#include "hal.h"
#include "hal_interface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include <time.h>
//#include <stdarg.h>
//#include <sys/time.h>
#include "esp_timer.h"

#define TAG "HAL"

/** 
 * @brief Wait specified time in milisecond
 *
 * @param[in] msec in milisecond
 *
 * @return none
 */
void HAL_Delay(int msec)
{
	esp_rom_delay_us(msec*1000);
	//vTaskDelay(pdMS_TO_TICKS(msec));
}

/** 
 * @brief get the current sys time in microsecond
 *
 * @param[in] msec in milisecond
 *
 * @return current sys time, 64-bit
 */
uint64_t HAL_GetTime64(void)
{
//   struct timeval time;
//   uint64_t time_output;
//
//   gettimeofday(&time,NULL);
//   time_output = (uint64_t)time.tv_usec + ((uint64_t)time.tv_sec)*1000000;
//   return time_output;
   return esp_timer_get_time();
}

/** 
 * @brief no operation
 *
 * @param none
 *
 * @return none
 */
void HAL_Nop(void)
{
   //no operation
}

/** 
 * @brief acquire current device number
 *
 * @param none
 *
 * @return current device number
 */
int HAL_DevNum(uint8_t * spidev)
{   
   return HAL_SPI_Which(spidev);
}


