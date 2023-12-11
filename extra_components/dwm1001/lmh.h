#pragma once
#include "esp_system.h"
#include "dwm_api.h"
#include "hal.h"
#include "hal_interface.h"

#define LMH_OK    HAL_OK
#define LMH_ERR   HAL_ERR

#include "lmh_spirx.h"
#include "lmh_spirx_drdy.h"

void LMH_HW_Init(void);
void LMH_Init(uint8_t * spidev);
void LMH_DeInit(uint8_t * spidev);
int LMH_Tx(uint8_t * spidev, uint8_t* data, uint8_t* length);
int LMH_WaitForRx(uint8_t * spidev, uint8_t* data, uint16_t* length, uint16_t exp_length);
int LMH_CheckRetVal(uint8_t* ret_val);
int LMH_CheckGPIOIdx(dwm_gpio_idx_t idx);
