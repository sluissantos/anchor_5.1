/*! ------------------------------------------------------------------------------------------------------------------
 * @file    lmh_spirx.c
 * @brief   low-level module handshake (LMH) utilities to handle DWM1001 data 
 *          transmission and receiving over SPI interface
 *          Use LMH_SPIRX_Init() before using to initialize the utilities. 
 *          Use LMH_SPIRX_WaitForRx() to wait for response message
 *
 *          In Makefile, interface configuration needs to be defined as:
 *          INTERFACE_NUMBER = 1
 *
 *          This file describes the RX setup example. 
 *
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "hal.h"
#include "lmh.h"
#include "hal_gpio.h"
#include "lmh_spirx.h"

#define TAG "LMH_SPIRX"

//#define LMH_SPIRX_DEBUG

#define LMH_SPIRX_HEADER_LENGTH           1
#define LMH_SPIRX_SIZE_OFFSET             0
#if LMH_SPIRX_HEADER_LENGTH == 2
#define LMH_SPIRX_NUM_OFFSET              1
#endif   

#define LMH_SPIRX_TIMEOUT_DEFAULT         1000

static bool lmh_spirx_initialized[2] = { false, false };
static int lmh_spirx_timeout = LMH_SPIRX_TIMEOUT_DEFAULT;
static int lmh_spirx_wait = HAL_SPI_WAIT_PERIOD;

/**
 * @brief : initialises the SPIRX functions. 
 */
void LMH_SPI_BUS_Init(void) {
	HAL_SPI_BUS_Init();
}

/**
 * @brief : initialises the SPIRX functions.
 */
void LMH_SPIRX_Init(uint8_t * spidev) {
	int dev = HAL_SPI_Which(spidev);
	if (dev > 1) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "Cannot find SPI dev%d.", dev);
#endif
		return;
	}
	if (lmh_spirx_initialized[dev]) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "LMH_SPIRX dev%d already initialized.", dev);
#endif

		return;
	}
	HAL_SPI_Init(spidev);

	LMH_SPIRX_SetTimeout(LMH_SPIRX_TIMEOUT_DEFAULT);
	LMH_SPIRX_SetWait(HAL_SPI_WAIT_PERIOD);
	LMH_SPIRX_SetToIdle(spidev);

	lmh_spirx_initialized[dev] = true;


#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "LMH_SPIRX_Init for SPI dev%d done.", dev);
#endif
}

/**
 * @brief : de-initialises the SPIRX functions. 
 */
void LMH_SPIRX_DeInit(uint8_t * spidev) {
	HAL_SPI_DeInit(spidev);
	lmh_spirx_initialized[HAL_SPI_Which(spidev)] = false;
}

/**
 * @brief : sets the DWM1001 module SPIRX functions into idle mode. 
 */
void LMH_SPIRX_SetToIdle(uint8_t * spidev) {
	uint8_t dummy = 0xff, length = 1;
	int i = 3;
#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "Reseting DWM1001 to SPI:IDLE");
#endif

	while (i-- > 0) {
		HAL_SPI_Tx(spidev,&dummy, &length);
		HAL_Delay(lmh_spirx_wait);
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGI(TAG, "Wait %d ms...", lmh_spirx_wait);
#endif

	}
}

/**
 * @brief : Set the SPIRX time out period. 
 *
 * @param [in] timeout, SPIRX time out period in ms
 */
void LMH_SPIRX_SetTimeout(int timeout) {
	lmh_spirx_timeout = timeout;
}

/**
 * @brief : Set the SPIRX wait period between each poll of SIZE. 
 *
 * @param [in] wait, SPIRX wait period in ms
 */
void LMH_SPIRX_SetWait(int wait) {
	lmh_spirx_wait = wait;
}

/**
 * @brief : wait length=exp_length for max time=lmh_spirx_wait
 *          needs LMH_SPIRX_Init() at initialization 
 *
 * @param [out] data,       pointer to received data 
 * @param [out] length,     pointer to received data length 
 * @param [in] exp_length,  expected data length
 *
 * @return Error code
 */
int LMH_SPIRX_WaitForRx(uint8_t * spidev, uint8_t *data, uint16_t *length, uint16_t exp_length) {
	uint8_t len_header, sizenum[LMH_SPIRX_HEADER_LENGTH];
	int timeout = lmh_spirx_timeout;

	int dev = HAL_SPI_Which(spidev);
	if (!lmh_spirx_initialized[dev]) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "SPI%d: LMH_SPIRX not initialized.", dev);
#endif
		return LMH_ERR;
	}
	if (exp_length < DWM1001_TLV_RET_VAL_MIN_SIZE) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "SPI%d: exp_length must be >= 3.", dev);
#endif
		return LMH_ERR;
	}
#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "SPI%d: Rx step 1:", dev);
#endif
	memset(sizenum, 0, LMH_SPIRX_HEADER_LENGTH);
	while ((sizenum[LMH_SPIRX_SIZE_OFFSET] == 0) && (timeout > 0)) {
		HAL_Delay(lmh_spirx_wait);
		timeout -= lmh_spirx_wait;
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGI(TAG, "SPI%d: Wait %d ms..., timeout=%d", dev, lmh_spirx_wait,
				timeout);
#endif
		len_header = LMH_SPIRX_HEADER_LENGTH;
		HAL_SPI_Rx(spidev,sizenum, &len_header);
	}
#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "sizenum: %d", sizenum[0]);
#endif
	if ((timeout < 0) && (sizenum[LMH_SPIRX_SIZE_OFFSET] == 0)) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "SPI%d: Read SIZE timed out after %d ms...", dev,
				lmh_spirx_timeout);
#endif
		return LMH_ERR;
	}

	*length = 0;
#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "SPI%d: Rx step 2:", dev);
#endif

#if LMH_SPIRX_HEADER_LENGTH == 2
   uint8_t i;
   for(i = 0; i < sizenum[LMH_SPIRX_NUM_OFFSET]; i++)
#endif
	{
		HAL_Delay(lmh_spirx_wait);
		timeout -= lmh_spirx_wait;
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGI(TAG, "SPI%d: Wait %d ms...", dev, lmh_spirx_wait);
#endif
		HAL_SPI_Rx(spidev, data, sizenum + LMH_SPIRX_SIZE_OFFSET);
		*length += sizenum[LMH_SPIRX_SIZE_OFFSET];
	}

	HAL_Delay(lmh_spirx_wait);
#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "SPI%d: Wait %d ms...", dev, lmh_spirx_wait);
#endif
	if (LMH_CheckRetVal(data) != LMH_OK) {
		return LMH_ERR;
	}

	if ((*length != exp_length) && (exp_length != DWM1001_TLV_MAX_SIZE)) {
#ifdef LMH_SPIRX_DEBUG
		ESP_LOGE(TAG, "SPI%d: Expecting %d bytes, received %d bytes, in %d ms",
				dev, exp_length, *length, lmh_spirx_timeout - timeout);
#endif
		return LMH_ERR;
	}

#ifdef LMH_SPIRX_DEBUG
	ESP_LOGI(TAG, "SPI%d: Received %d bytes, in %d ms \t OK", dev, *length,
			lmh_spirx_timeout - timeout);
#endif
	return LMH_OK;
}
