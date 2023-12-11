#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "lmh.h"
#include "esp_log.h"

#include "hal_gpio.h"
#include "hal.h"
#include "lmh_spirx.h"

#define TAG "LMH_SPIRX_DRDY"

//#define LMH_SPIRX_DRDY_DEBUG

//#define LMH_SPIRX_DRDY_HEADER_LENGTH           1
#define LMH_SPIRX_DRDY_HEADER_LENGTH           2
#define LMH_SPIRX_DRDY_SIZE_OFFSET             0
#if LMH_SPIRX_DRDY_HEADER_LENGTH == 2
#define LMH_SPIRX_DRDY_NUM_OFFSET              1
#endif   

#define LMH_SPIRX_DRDY_TIMEOUT_DEFAULT       1000

int LMH_SPIRX_DRDY_IntCfg(uint8_t * spidev, uint16_t value);
bool LMH_SPIRX_DRDY_PinGet(void);

static bool lmh_spirx_drdy_initialized[2] = { false, false };
static int lmh_spirx_drdy_timeout = LMH_SPIRX_DRDY_TIMEOUT_DEFAULT;
static int lmh_spirx_drdy_wait = HAL_SPI_WAIT_PERIOD;
static bool lmh_spirx_drdy_drdy_flag = false;


/**
 * @brief : initialises the UARTRX functions. 
 */
void LMH_SPIRX_DRDY_Init(uint8_t * spidev) {
	int dev = HAL_SPI_Which(spidev);
	if (dev > 1) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "Cannot find SPI dev%d.", dev);
#endif
		return;
	}
	if (lmh_spirx_drdy_initialized[dev]) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "LMH_SPIRX_DRDY dev%d already initialized.", dev);
#endif
		return;
	}
	LMH_SPIRX_Init(spidev);
	//if (LMH_SPIRX_DRDY_IntCfg(DWM1001_INTR_SPI_DATA_READY | DWM1001_INTR_UWBMAC_STATUS_CHANGED | DWM1001_INTR_UWBMAC_BH_DATA_READY | DWM1001_INTR_UWBMAC_BH_AVAILABLE) == LMH_ERR) {
	if (LMH_SPIRX_DRDY_IntCfg(spidev,DWM1001_INTR_SPI_DATA_READY) == LMH_ERR) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "LMH_SPIRX_DRDY_IntCfg() failed.");
#endif

		return;
	}

	HAL_GPIO_Init();
	if(dev==HAL_SPI_DEV1) HAL_GPIO_SetupCb(HAL_GPIO_DRDY1, HAL_GPIO_INT_EDGE_BOTH, (void *)&LMH_SPIRX_DRDY_DrdyCb);
	else HAL_GPIO_SetupCb(HAL_GPIO_DRDY0, HAL_GPIO_INT_EDGE_BOTH, (void *)&LMH_SPIRX_DRDY_DrdyCb);

	LMH_SPIRX_DRDY_SetTimeout(LMH_SPIRX_DRDY_TIMEOUT_DEFAULT);
	LMH_SPIRX_DRDY_SetWait(HAL_SPI_WAIT_PERIOD);
	LMH_SPIRX_DRDY_PinCheck(spidev);
	LMH_SPIRX_DRDY_SetToIdle(spidev);

	lmh_spirx_drdy_initialized[dev] = true;
}

/**
 * @brief : de-initialises the UARTRX functions. 
 */
void LMH_SPIRX_DRDY_DeInit(uint8_t * spidev) {
	int dev = HAL_SPI_Which(spidev);
	LMH_SPIRX_DeInit(spidev);
	lmh_spirx_drdy_initialized[dev] = false;
	if(dev==HAL_SPI_DEV1) HAL_GPIO_SetupCb(HAL_GPIO_DRDY1, HAL_GPIO_INT_EDGE_SETUP, NULL);
	else HAL_GPIO_SetupCb(HAL_GPIO_DRDY0, HAL_GPIO_INT_EDGE_SETUP, NULL);
}

/**
 * @brief : send the command to configure DWM1001 to enable Data Ready pin
 *
 * @return Error code
 */
int LMH_SPIRX_DRDY_IntCfg(uint8_t * spidev, uint16_t value) {
	uint8_t tx_data[DWM1001_TLV_MAX_SIZE], tx_len = 0;
	uint8_t rx_data[DWM1001_TLV_MAX_SIZE];
	uint16_t rx_len;
	tx_data[tx_len++] = DWM1001_TLV_TYPE_CMD_INT_CFG_SET;
	tx_data[tx_len++] = 2;
	tx_data[tx_len++] = value & 0xff;
	tx_data[tx_len++] = (value >> 8) & 0xff;
	HAL_SPI_Tx(spidev,tx_data, &tx_len);
	//return LMH_ERR;
	return LMH_SPIRX_WaitForRx(spidev,rx_data, &rx_len, 3);
}

/**
 * @brief : sets the DWM1001 module SPIRX functions into idle mode. 
 */
void LMH_SPIRX_DRDY_SetToIdle(uint8_t * spidev) {
	uint8_t dummy = 0xff, length = 1;
	int i = 3;
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "Reseting DWM1001 to SPI:IDLE ");
#endif
	while (i-- > 0) {
		HAL_SPI_Tx(spidev,&dummy, &length);
		HAL_Delay(1);
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGI(TAG, "Wait %d ms...", 1);
#endif
	}
}

/**
 * @brief : on drdy pin going high/low, set/reset the lmh_spirx_drdy_drdy_flag 
 *
 * @return none
 */

void LMH_SPIRX_DRDY_DrdyCb(uint8_t * spidev)
{
//	dwm_status_t status;
	LMH_SPIRX_DRDY_PinCheck(spidev);
//	dwm_status_get(&status);
#ifdef LMH_SPIRX_DRDY_DEBUG
	//ESP_LOGI(TAG,"DRDY pin change detected.  %s ", LMH_SPIRX_DRDY_PinGet() ? "+++" : "---");
	//printf("DRDY pin change detected.  %s \n", LMH_SPIRX_DRDY_PinGet() ? "+++" : "---");
//
//	ESP_LOGI(TAG,"\t\tstatus.loc_data            = %d", status.loc_data);
//	ESP_LOGI(TAG,"\t\tstatus.uwbmac_joined       = %d", status.uwbmac_joined);
//	ESP_LOGI(TAG,"\t\tstatus.bh_data_ready       = %d", status.bh_data_ready);
//	ESP_LOGI(TAG,"\t\tstatus.bh_status_changed   = %d", status.bh_status_changed);
//	ESP_LOGI(TAG,"\t\tstatus.bh_initialized      = %d", status.bh_initialized);
//	ESP_LOGI(TAG,"\t\tstatus.uwb_scan_ready      = %d", status.uwb_scan_ready);
//	ESP_LOGI(TAG,"\t\tstatus.usr_data_ready      = %d", status.usr_data_ready);
//	ESP_LOGI(TAG,"\t\tstatus.usr_data_sent       = %d", status.usr_data_sent);
//	ESP_LOGI(TAG,"\t\tstatus.fwup_in_progress    = %d", status.fwup_in_progress);
//
//	bh_status_t bh_status;
//	int i;
//	dwm_bh_status_get(&bh_status);
//
//	ESP_LOGI(TAG,"\t\tbh_status.sf_number  = %d", bh_status.sf_number);
//	ESP_LOGI(TAG,"\t\tbh_status.bh_seat_map  = %d", bh_status.bh_seat_map);
//	ESP_LOGI(TAG,"\t\tbh_status.origin_cnt  = %d", bh_status.origin_cnt);
//	for (i = 0; i < bh_status.origin_cnt; i++)
//	{
//		ESP_LOGI(TAG,"\t\t\tbh_status.origin_info[%d].node_id  = %d", i, bh_status.origin_info[i].node_id);
//		ESP_LOGI(TAG,"\t\t\tbh_status.origin_info[%d].bh_seat  = %d", i, bh_status.origin_info[i].bh_seat);
//		ESP_LOGI(TAG,"\t\t\tbh_status.origin_info[%d].hop_lvl  = %d", i, bh_status.origin_info[i].hop_lvl);
//	}
//	ESP_LOGI(TAG,"");
#endif



}

/**
 * @brief : update lmh_spirx_drdy_drdy_flag status according to HAL_GPIO_DRDY status
 *
 * @return none
 */
void LMH_SPIRX_DRDY_PinCheck(uint8_t * spidev) {
	if(*spidev==HAL_SPI_DEV1) lmh_spirx_drdy_drdy_flag = HAL_GPIO_PinRead(HAL_GPIO_DRDY1) == 1 ? true : false;
	else lmh_spirx_drdy_drdy_flag = HAL_GPIO_PinRead(HAL_GPIO_DRDY0) == 1 ? true : false;
}

/**
 * @brief : get lmh_spirx_drdy_drdy_flag 
 *
 * @return lmh_spirx_drdy_drdy_flag
 */
bool LMH_SPIRX_DRDY_PinGet(void) {
	return lmh_spirx_drdy_drdy_flag;
}

/**
 * @brief : Set the SPIRX_DRDY time out period. 
 *
 * @param [in] timeout, SPIRX_DRDY time out period in ms
 */
void LMH_SPIRX_DRDY_SetTimeout(int timeout) {
	lmh_spirx_drdy_timeout = timeout;
}

/**
 * @brief : Set the SPIRX_DRDY wait period between each poll of SIZE. 
 *
 * @param [in] wait, SPIRX_DRDY wait period in ms
 */
void LMH_SPIRX_DRDY_SetWait(int wait) {
	lmh_spirx_drdy_wait = wait;
}

/**
 * @brief : wait length=exp_length for max time=lmh_spirx_drdy_wait
 *          needs LMH_SPIRX_DRDY_Init() at initialization 
 *
 * @param [out] data,       pointer to received data 
 * @param [out] length,     pointer to received data length 
 * @param [in] exp_length,  expected data length
 *
 * @return Error code
 */
int LMH_SPIRX_DRDY_WaitForRx(uint8_t * spidev, uint8_t *data, uint16_t *length,
		uint16_t exp_length) {
	uint8_t len_header, sizenum[LMH_SPIRX_DRDY_HEADER_LENGTH];
	int timeout = lmh_spirx_drdy_timeout;
	int ret_val = LMH_OK;
	int wait_temp;

	// check invalid cases
	int dev = HAL_SPI_Which(spidev);
	if (!lmh_spirx_drdy_initialized[dev]) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "LMH_SPIRX_DRDY dev%d not initialized.", dev);
#endif
		return LMH_ERR;
	}
	if (exp_length < DWM1001_TLV_RET_VAL_MIN_SIZE) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "exp_length must be >= 3");
#endif
		return LMH_ERR;
	}
	// wait for SIZE to be ready
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Rx step 1:", dev);
#endif
	memset(sizenum, 0, LMH_SPIRX_DRDY_HEADER_LENGTH);
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Start wait for spirx_drdy ...", dev);
#endif
	wait_temp = 0;
//	ESP_LOGI(TAG, "LMH_SPIRX_DRDY_PinGet:%d", LMH_SPIRX_DRDY_PinGet());
	while ((!LMH_SPIRX_DRDY_PinGet()) && (timeout > 0)) {
		HAL_Delay(lmh_spirx_drdy_wait);
		wait_temp += lmh_spirx_drdy_wait;
		timeout -= lmh_spirx_drdy_wait;
		LMH_SPIRX_DRDY_PinCheck(spidev);
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Waited %d ms for spirx_drdy ...", dev,
			wait_temp);

	ESP_LOGI(TAG, "SPI_DRDY%d: Wait %d ms...", dev, lmh_spirx_drdy_wait);
#endif

	HAL_Delay(lmh_spirx_drdy_wait);
	timeout -= lmh_spirx_drdy_wait;

	if (timeout <= 0) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "Read SIZE timed out after %d ms...",
				lmh_spirx_drdy_timeout);
#endif
		LMH_SPIRX_DRDY_PinCheck(spidev);
		ret_val = LMH_ERR;
	}

	//---------- read SIZE & NUM
	len_header = LMH_SPIRX_DRDY_HEADER_LENGTH;
	HAL_SPI_Rx(spidev,sizenum, &len_header);
#ifdef LMH_SPIRX_DRDY_DEBUG
	#if LMH_SPIRX_DRDY_HEADER_LENGTH == 2
	ESP_LOGI(TAG, "SIZE: %d, NUM: %d", sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET],sizenum[LMH_SPIRX_DRDY_NUM_OFFSET]);
	#else
	ESP_LOGI(TAG, "SIZE: %d", sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET]);
	#endif
#endif

	//---------- wait for DATA to be ready
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Rx step 2: ", dev);
	ESP_LOGI(TAG, "SPI_DRDY%d: Start wait for spirx_drdy ...", dev);
#endif
	wait_temp = 0;
	while ((!LMH_SPIRX_DRDY_PinGet()) && (timeout > 0)) {
		HAL_Delay(lmh_spirx_drdy_wait);
		timeout -= lmh_spirx_drdy_wait;
		LMH_SPIRX_DRDY_PinCheck(spidev);
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Waited %d ms for spirx_drdy ...", dev,wait_temp);
#endif

	//---------- read DATA
	*length = 0;
#if LMH_SPIRX_DRDY_HEADER_LENGTH == 2
   uint8_t i;
   for(i = 0; i < sizenum[LMH_SPIRX_DRDY_NUM_OFFSET]; i++)
#endif
	{
		HAL_Delay(lmh_spirx_drdy_wait);
		timeout -= lmh_spirx_drdy_wait;
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGI(TAG, "SPI_DRDY%d: Wait %d ms...", dev, lmh_spirx_drdy_wait);
#endif
		HAL_SPI_Rx(spidev, data, sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET);
		*length += sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET];

		//printf("T: 0x%02x, L: 0x%02x\n",data[0],data[1]);
	}

	HAL_Delay(lmh_spirx_drdy_wait);
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "Wait %d ms...", lmh_spirx_drdy_wait);
#endif

	LMH_SPIRX_DRDY_PinCheck(spidev);

	//printf("len:%d\n",*length);
	if (LMH_CheckRetVal(data) != LMH_OK) {
		return LMH_ERR;
	}

	if ((*length != exp_length) && (exp_length != DWM1001_TLV_MAX_SIZE)) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "Expecting %d bytes, received %d bytes, in %d ms",
				exp_length, *length, lmh_spirx_drdy_timeout - timeout);
#endif

		return LMH_ERR;
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "Received %d bytes, in %d ms", *length, lmh_spirx_drdy_timeout - timeout);
	//Mostrar os bytes recebidos aqui.
#endif
	return ret_val;

}

///////---------------------------------------------------------------------------------

int LMH_SPIRX_XFER_DRDY_WaitForRx(uint8_t * spidev, uint8_t *rx_data, uint16_t *effective_len, uint16_t exp_length) {
	uint8_t len_header, sizenum[LMH_SPIRX_DRDY_HEADER_LENGTH];
	int timeout = lmh_spirx_drdy_timeout;
	int ret_val = LMH_OK;
	int wait_temp;

	// check invalid cases
	int dev = HAL_SPI_Which(spidev);
	if (!lmh_spirx_drdy_initialized[dev]) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "LMH_SPIRX_DRDY dev%d not initialized.", dev);
#endif
		return LMH_ERR;
	}
	if (exp_length < DWM1001_TLV_RET_VAL_MIN_SIZE) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "exp_length must be >= 3");
#endif
		return LMH_ERR;
	}
	// wait for SIZE to be ready
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Rx step 1:", dev);
#endif
	memset(sizenum, 0, LMH_SPIRX_DRDY_HEADER_LENGTH);
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Start wait for spirx_drdy ...", dev);
#endif
	wait_temp = 0;
//	ESP_LOGI(TAG, "LMH_SPIRX_DRDY_PinGet:%d", LMH_SPIRX_DRDY_PinGet());
	while ((!LMH_SPIRX_DRDY_PinGet()) && (timeout > 0)) {
		HAL_Delay(lmh_spirx_drdy_wait);
		wait_temp += lmh_spirx_drdy_wait;
		timeout -= lmh_spirx_drdy_wait;
		LMH_SPIRX_DRDY_PinCheck(spidev);
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Waited %d ms for spirx_drdy ...", dev,
			wait_temp);

	ESP_LOGI(TAG, "SPI_DRDY%d: Wait %d ms...", dev, lmh_spirx_drdy_wait);
#endif

	HAL_Delay(lmh_spirx_drdy_wait);
	timeout -= lmh_spirx_drdy_wait;

	if (timeout <= 0) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "Read SIZE timed out after %d ms...",
				lmh_spirx_drdy_timeout);
#endif
		LMH_SPIRX_DRDY_PinCheck(spidev);
		ret_val = LMH_ERR;
	}

	//---------- read SIZE & NUM
	len_header = LMH_SPIRX_DRDY_HEADER_LENGTH;
	HAL_SPI_Rx(spidev, sizenum, &len_header);
#ifdef LMH_SPIRX_DRDY_DEBUG
	#if LMH_SPIRX_DRDY_HEADER_LENGTH == 2
	ESP_LOGI(TAG, "SIZE: %d, NUM: %d", sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET],sizenum[LMH_SPIRX_DRDY_NUM_OFFSET]);
	#else
	ESP_LOGI(TAG, "SIZE: %d", sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET]);
	#endif
#endif

	//---------- wait for DATA to be ready
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Rx step 2: ", dev);
	ESP_LOGI(TAG, "SPI_DRDY%d: Start wait for spirx_drdy ...", dev);
#endif
	wait_temp = 0;
	while ((!LMH_SPIRX_DRDY_PinGet()) && (timeout > 0)) {
		HAL_Delay(lmh_spirx_drdy_wait);
		timeout -= lmh_spirx_drdy_wait;
		LMH_SPIRX_DRDY_PinCheck(spidev);
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "SPI_DRDY%d: Waited %d ms for spirx_drdy ...", dev,wait_temp);
#endif

	//---------- read DATA
	uint16_t length = 0;
	uint8_t data[255];
#if LMH_SPIRX_DRDY_HEADER_LENGTH == 2
   uint8_t i;
  // uint8_t j;
   *effective_len=0;

//   uint8_t tx_data[255];
//   memset(tx_data, 0xFF, *(sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET));
//   tx_data[0]=0x64;
//   tx_data[1]=0x01;
//   tx_data[2]=0x02;
//
//   HAL_SPI_Tx(tx_data, sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET);
//   memset(tx_data, 0xFF, *(sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET));



   for(i = 0; i < sizenum[LMH_SPIRX_DRDY_NUM_OFFSET]; i++)
#endif
	{
	    memset(data,0,255*sizeof(uint8_t));
		HAL_Delay(lmh_spirx_drdy_wait);
		timeout -= lmh_spirx_drdy_wait;
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGI(TAG, "SPI_DRDY%d: Wait %d ms...", dev, lmh_spirx_drdy_wait);
#endif
//		HAL_SPI_XFER_Rx(data, sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET,tx_data);
		HAL_SPI_Rx(spidev, data, sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET);
		length += sizenum[LMH_SPIRX_DRDY_SIZE_OFFSET];
		memcpy(rx_data+*effective_len,data+2,data[1]);
		*effective_len += data[1];

//		memset(tx_data, 0xFF, *(sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET));
//		HAL_SPI_Tx(tx_data, sizenum + LMH_SPIRX_DRDY_SIZE_OFFSET);
	}

	HAL_Delay(lmh_spirx_drdy_wait);
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "Wait %d ms...", lmh_spirx_drdy_wait);
#endif

	LMH_SPIRX_DRDY_PinCheck(spidev);

	if ((length != exp_length) && (exp_length != 5*DWM1001_TLV_MAX_SIZE)) {
#ifdef LMH_SPIRX_DRDY_DEBUG
		ESP_LOGE(TAG, "Expecting %d bytes, received %d bytes, in %d ms",
				exp_length, length, lmh_spirx_drdy_timeout - timeout);
		printf("Expecting %d bytes, received %d bytes, in %d ms\n",
						exp_length, length, lmh_spirx_drdy_timeout - timeout);
#endif

		return LMH_ERR;
	}
#ifdef LMH_SPIRX_DRDY_DEBUG
	ESP_LOGI(TAG, "Received %d bytes, in %d ms", *effective_len, lmh_spirx_drdy_timeout - timeout);
#endif
	return ret_val;

}
