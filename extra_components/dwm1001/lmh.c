#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "lmh_spirx_drdy.h"
#include "hal_interface.h"
#include "dwm1001_tlv.h"
#include "dwm_api.h"
#include "lmh.h"

#define TAG "LMH"

//#define LMH_DEBUG

void LMH_HW_Init(void){
	LMH_SPI_BUS_Init();
}

/**
 * @brief initializes the LMH utilities over defined interface
 *
 * @param none
 *
 * @return none
 */
void LMH_Init(uint8_t * spidev)
{
	#ifdef LMH_DEBUG
	ESP_LOGI(TAG, "LMH_Init...");
	#endif
	//LMH_SPIRX_Init();
	LMH_SPIRX_DRDY_Init(spidev);
}

/**
 * @brief de-initializes the LMH utilities over defined interface
 *
 * @param none
 *
 * @return none
 */
void LMH_DeInit(uint8_t * spidev)
{
	#ifdef LMH_DEBUG
	ESP_LOGI(TAG, "LMH_DeInit...");
	#endif
	//LMH_SPIRX_DeInit();
	LMH_SPIRX_DRDY_DeInit(spidev);
}

/**
 * @brief transmit data over defined interface
 *
 * @param [in] data: pointer to the Tx data buffer
 * @param [in] length: length of data to be received
 *
 * @return Error code
 */
int LMH_Tx(uint8_t * spidev, uint8_t* data, uint8_t* length)
{
   return HAL_IF_Tx(spidev, data, length);
}

/**
 * @brief wait for response data over defined interface
 *       note: this function is blocking
 *
 * @param [out] data: pointer to the RX data buffer
 * @param [out] length: length of data to be received
 * @param [in] exp_length: expected length of response data.
 *             Note - If the user doesn't know how long the response from DWM1001 to the host
 *                   is, then this parameter should be set to DWM1001_TLV_MAX_SIZE as defined
 *                   in dwm1001.h. In this case,
 *                   for SPI, length check won't report error no matter how long the received
 *                   data is;
 *                   for UART, this function will not return until the timeout period expires.
 *
 * @return Error code
 */
int LMH_WaitForRx(uint8_t * spidev, uint8_t* data, uint16_t* length, uint16_t exp_length)
{
	//return LMH_SPIRX_WaitForRx(data, length, exp_length);
	return LMH_SPIRX_DRDY_WaitForRx(spidev, data, length, exp_length);
}

/**
 * @brief wait for response data over defined interface
 *       note: this function is blocking
 *
 * @param [in] ret_val: pointer to the response data buffer, where the first three bytes must
 *       be TLV values 0x40, 0x01, 0x00 meaning a RV_OK, to indicating that the request is
 *       properly parsed. Otherwise the previous communication between the host and DWM1001
 *       was not acting correctly.
 *
 * @return Error code
 */
int LMH_CheckRetVal(uint8_t* ret_val)
{
   if(ret_val[0] != DWM1001_TLV_TYPE_RET_VAL)
   {
	   #ifdef LMH_DEBUG
	   ESP_LOGE(TAG,"%s: RET_VAL type wrong: %d", HAL_IF_STR, ret_val[0]);
		#endif
      return LMH_ERR;
   }
   if(ret_val[1] != 1)
   {
		#ifdef LMH_DEBUG
      ESP_LOGE(TAG,"%s: RET_VAL length wrong: %d", HAL_IF_STR, ret_val[1]);
		#endif
      return LMH_ERR;
   }

   if(ret_val[2] == RV_OK)
   {
      return LMH_OK;
   }
   else
   {
	#ifdef LMH_DEBUG
      ESP_LOGE(TAG,"%s: DWM1001_RV_ERR: %d", HAL_IF_STR, ret_val[2]);
	#endif
      return LMH_ERR;
   }
}

/**
 * @brief check if GPIO pin: index is among available pins
 *
 * @param[in] idx, GPIO pin index
 *
 * @return Error code
 */
int LMH_CheckGPIOIdx(dwm_gpio_idx_t idx)
{
   if((idx == 2 ) || (idx == 8 ) || (idx == 9 ) || (idx == 10) || (idx == 12)  \
   || (idx == 13) || (idx == 14) || (idx == 15) || (idx == 22) || (idx == 23)  \
   || (idx == 27) || (idx == 30) || (idx == 31))
   {
      return LMH_OK; //good
   }
   else
   {
      return LMH_ERR; //error
   }
}
