/*! ------------------------------------------------------------------------------------------------------------------
 * @file    hal_spi.c
 * @brief   utility to operate spi device based on Linux system
 *
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */
#include <stdbool.h>
#include <fcntl.h>   // for open
#include <unistd.h>  // for close
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include "esp_system.h"
#include "driver/spi_master.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal_spi.h"
#include "hal.h"
#include "freertos/semphr.h"
#include "config.h"

#define TAG "HAL_SPI"

#define HAL_SPI_DEBUG

#define HAL_SPI_BITS                 8
#define HAL_SPI_SPEED                1000000
#define HAL_SPI_DELAY                0

static spi_device_handle_t spi_dev1 = NULL;
static spi_device_handle_t spi_dev2 = NULL;
static spi_transaction_t tr_data;
static unsigned char spi_tx;
static unsigned char spi_rx;

static SemaphoreHandle_t spi_mux = NULL;

void HAL_SPI_BUS_Init(void) {
	esp_err_t ret;
	spi_bus_config_t buscfg = {
			.miso_io_num = AURA_MISO_PIN,
			.mosi_io_num = AURA_MOSI_PIN,
			.sclk_io_num = AURA_CLK_PIN,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1 };
	//ret = spi_bus_initialize(VSPI_HOST, &buscfg, 0);
	ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1); //Enable DMA
	ESP_ERROR_CHECK(ret);
#ifdef HAL_SPI_DEBUG
	ESP_LOGI(TAG, "spi_bus_initialize OK");
#endif
	spi_mux = xSemaphoreCreateMutex();
}

/** 
 * @brief initializes the current SPI device, default /dev/spidev0.0
 *        use HAL_SPI_Sel to set current spi device
 *        use HAL_SPI_Which to get current spi device
 *
 * @param none
 *
 * @return Error code
 */
int HAL_SPI_Init(uint8_t * spidev) {
	esp_err_t ret;

	if(*spidev==HAL_SPI_DEV0){
		spi_device_interface_config_t devcfg1 = {
				//.command_bits = 0,
				//.address_bits = 0,
				//.dummy_bits = 0,
				.clock_speed_hz = HAL_SPI_SPEED,
				.duty_cycle_pos = 128,
				.mode = 0,
				.spics_io_num = AURA1_CS_PIN,
				.queue_size = 10,
				.input_delay_ns = HAL_SPI_DELAY,
				.cs_ena_pretrans = 10,
				.cs_ena_posttrans = 10,
		};
		ret = spi_bus_add_device(VSPI_HOST, &devcfg1, &spi_dev1);
		ESP_ERROR_CHECK(ret);
		#ifdef HAL_SPI_DEBUG
		ESP_LOGI(TAG, "spi_bus_add_device OK");
		#endif
	}
	else if(*spidev==HAL_SPI_DEV1){
		spi_device_interface_config_t devcfg2 = {
				//.command_bits = 0,
				//.address_bits = 0,
				//.dummy_bits = 0,
				.clock_speed_hz = HAL_SPI_SPEED,
				.duty_cycle_pos = 128,
				.mode = 0,
				.spics_io_num = AURA2_CS_PIN,
				.queue_size = 10,
				.input_delay_ns = HAL_SPI_DELAY,
				.cs_ena_pretrans = 10,
				.cs_ena_posttrans = 10,
		};
		ret = spi_bus_add_device(VSPI_HOST, &devcfg2, &spi_dev2);
		ESP_ERROR_CHECK(ret);
		#ifdef HAL_SPI_DEBUG
		ESP_LOGI(TAG, "spi_bus_add_device OK");
		#endif
	}
	else {
#ifdef HAL_SPI_DEBUG
	ESP_LOGE(TAG, "spidev not found!");
#endif
	}

	return 1;
}

/** 
 * @brief de-initializes the current SPI device
 *
 * @param none
 *
 * @return none
 */
void HAL_SPI_DeInit(uint8_t * spidev) {
	esp_err_t ret;
	if(*spidev==HAL_SPI_DEV1){
		ret = spi_bus_remove_device(spi_dev1);
		spi_dev1 = NULL;
		ESP_ERROR_CHECK(ret);
	}
	else if(*spidev==HAL_SPI_DEV1){
		ret = spi_bus_remove_device(spi_dev2);
		spi_dev2 = NULL;
		ESP_ERROR_CHECK(ret);
	}
#ifdef HAL_SPI_DEBUG
	ESP_LOGI(TAG, "bus remove device OK");
#endif
}

/** 
 * @brief set current spi device
 *
 * @param [in] spi dev number, 0 or 1
 *
 * @return none
 */
void HAL_SPI_Sel(int dev) {
//   const char *device_str;
//   curr_dev = dev;
//   device_str = dev ? device1 : device0;
//   curr_dev_fd = dev==0 ? spi_dev0_fd : spi_dev1_fd;
//   ESP_LOGI(TAG,"SPI%d: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Device select: %s", HAL_SPI_Which(), device_str);
}

/** 
 * @brief acquire current spi device
 *
 * @param none
 *
 * @return current spi device
 */
int HAL_SPI_Which(uint8_t * spidev) {
	return *spidev;   //curr_dev;
}

/** 
 * @brief transmit data of length over the current SPI device
 *
 * @param [in] data: pointer to the TX data
 * @param [in] length: length of data to be transmitted
 *
 * @return Error code
 */
int HAL_SPI_Tx(uint8_t * spidev, uint8_t *tx_data, uint8_t *length) {
	xSemaphoreTake(spi_mux, portMAX_DELAY);
	//uint16_t i;
	uint8_t rx_data[HAL_SPI_MAX_LENGTH];
	uint8_t tx_length = *length;
	//uint16_t str_len = 0;

	//int errno;
	//char print_str[HAL_SPI_MAX_PRINT_LENGTH];

	if (tx_length == 0) {
		return HAL_OK;
	}
//   if(tx_length > HAL_SPI_MAX_LENGTH){
//      ESP_LOGE(TAG,"SPI%d: Tx length exceeds the limit: %d", HAL_SPI_Which(), (uint16_t)HAL_SPI_MAX_LENGTH);
//      return HAL_ERR;
//   }

	//print TX data
//   errno = snprintf(print_str, HAL_SPI_MAX_PRINT_LENGTH, "SPI: Tx %d bytes: 0x", tx_length);
//   str_len = (errno >= 0)? strlen(print_str):0;
//   for(i = 0; i <tx_length; i++){
//      errno = snprintf(print_str+str_len, HAL_SPI_MAX_PRINT_LENGTH-str_len, "%02x", tx_data[i]);
//      str_len += (errno >= 0)? 2:0;
//   }
//   errno = snprintf(print_str+str_len, HAL_SPI_MAX_PRINT_LENGTH-str_len, "\n");
//   str_len += (errno >= 0)? 1:0;
//   ESP_LOGI(TAG,"%s", print_str);
	///////////////////////////////

//	tr_data.tx_buf = (unsigned long)tx_data;
//	tr_data.rx_buf = (unsigned long)rx_data;
//	tr_data.len = tx_length;
//
//	errno = ioctl(curr_dev_fd, SPI_IOC_MESSAGE(1), &tr_data);
//	if (errno == -1){
//		ESP_LOGE(TAG,"SPI%d: Error in %s", HAL_SPI_Which(), __func__);
//      return HAL_ERR;
//   }
//
	//ESP_LOGI(TAG, "SPI TX tx_data: 0x%02x 0x%02x 0x%02x 0x%02x, tx_length: %d", tx_data[0], tx_data[1],tx_data[2],tx_data[3], tx_length);
	esp_err_t ret;
	memset(&tr_data, 0, sizeof(tr_data));       //Zero out the transaction
	tr_data.length = tx_length * HAL_SPI_BITS; //Len is in bytes, transaction length is in bits.
	tr_data.tx_buffer = tx_data;               //Data
	tr_data.rx_buffer = rx_data;
	tr_data.user = (void*) 0;                //D/C needs to be set to 1
	if(*spidev == HAL_SPI_DEV1) ret = spi_device_polling_transmit(spi_dev2, &tr_data);  //Transmit!
	else ret = spi_device_polling_transmit(spi_dev1, &tr_data);  //Transmit!
	//printf("ret %x \n",ret);
	//t.user = (void*) 1;
	assert(ret == ESP_OK);            //Should have had no issues.

	//set back values to protect
	tr_data.tx_buffer = &spi_tx;
	tr_data.rx_buffer = &spi_rx;

	//ets_delay_us(100);
	xSemaphoreGive(spi_mux);
	return HAL_OK;
}

/** 
 * @brief receive data of length over the current SPI device
 *
 * @param [in] data: pointer to the RX data buffer
 * @param [in] length: length of data to be received
 *
 * @return Error code
 */
int HAL_SPI_Rx(uint8_t * spidev, uint8_t *rx_data, uint8_t *length) {
	xSemaphoreTake(spi_mux, portMAX_DELAY);
	//uint16_t i;
	uint8_t tx_data[HAL_SPI_MAX_LENGTH];
	uint8_t rx_length = *length;
	//uint16_t str_len = 0;
	//int errno;
	//char print_str[HAL_SPI_MAX_PRINT_LENGTH];

	if (rx_length == 0) {
		return HAL_OK;
	}

//   if(rx_length > HAL_SPI_MAX_LENGTH){
//      ESP_LOGE(TAG,"SPI%d: Rx length exceeds the limit: %d.", HAL_SPI_Which(), (uint16_t)HAL_SPI_MAX_LENGTH);
//      return HAL_ERR;
//   }

	memset(tx_data, 0x00, rx_length); //dont care. can be any byte not only 0xFF

//	tr_data.tx_buf = (unsigned long)tx_data;
//	tr_data.rx_buf = (unsigned long)rx_data;
//	tr_data.len = rx_length;
//
//	errno = ioctl(curr_dev_fd, SPI_IOC_MESSAGE(1), &tr_data);
//	if (errno == -1){
//		ESP_LOGE(TAG,"SPI%d: Error in %s", HAL_SPI_Which(), __func__);
//      return HAL_ERR;
//   }
//
//   //set back values to protect
//	tr_data.tx_buf = (unsigned long)&spi_tx;
//	tr_data.rx_buf = (unsigned long)&spi_rx;

	spi_transaction_t tr_data;
	memset(&tr_data, 0, sizeof(tr_data));
	tr_data.length = rx_length * HAL_SPI_BITS;
	tr_data.tx_buffer = tx_data;
	//tr_data.flags = SPI_TRANS_USE_RXDATA;
	tr_data.user = (void*) 1;
	//tr_data.rx_data;
	tr_data.rx_buffer = rx_data;
	esp_err_t ret;
	if(*spidev == HAL_SPI_DEV1) ret = spi_device_polling_transmit(spi_dev2, &tr_data);
	else ret = spi_device_polling_transmit(spi_dev1, &tr_data);

	assert(ret == ESP_OK);

	//set back values to protect
	tr_data.tx_buffer = &spi_tx;
	tr_data.rx_buffer = &spi_rx;

//   errno = snprintf(print_str, HAL_SPI_MAX_PRINT_LENGTH, "SPI: Rx %d bytes: 0x", rx_length);
//   str_len = (errno >= 0)? strlen(print_str):0;
//   for(i = 0; i <rx_length; i++){
//      errno = snprintf(print_str+str_len, HAL_SPI_MAX_PRINT_LENGTH-str_len, "%02x", rx_data[i]);
//      str_len += (errno >= 0)? 2:0;
//   }
//   errno = snprintf(print_str+str_len, HAL_SPI_MAX_PRINT_LENGTH-str_len, "\n");
//   str_len += (errno >= 0)? 1:0;
//   ESP_LOGI(TAG,"%s", print_str);

	//ets_delay_us(100);
	xSemaphoreGive(spi_mux);
	return HAL_OK;
}

int HAL_SPI_XFER_Rx(uint8_t * spidev, uint8_t *rx_data, uint8_t *length, uint8_t *tx_data) {
	xSemaphoreTake(spi_mux, portMAX_DELAY);
	//uint8_t tx_data[HAL_SPI_MAX_LENGTH];
	uint8_t rx_length = *length;

	if (rx_length == 0) {
		return HAL_OK;
	}

	//memset(tx_data, 0xFF, rx_length); //dont care. can be any byte not only 0xFF

	spi_transaction_t tr_data;
	memset(&tr_data, 0, sizeof(tr_data));
	tr_data.length = rx_length * HAL_SPI_BITS;
	tr_data.tx_buffer = tx_data;
	//tr_data.flags = SPI_TRANS_USE_RXDATA;
	tr_data.user = (void*) 1;
	//tr_data.rx_data;
	tr_data.rx_buffer = rx_data;
	esp_err_t ret;
	if(*spidev == HAL_SPI_DEV1) ret = spi_device_polling_transmit(spi_dev2, &tr_data);
	else ret = spi_device_polling_transmit(spi_dev1, &tr_data);
	assert(ret == ESP_OK);

	//set back values to protect
	tr_data.tx_buffer = &spi_tx;
	tr_data.rx_buffer = &spi_rx;

	esp_rom_delay_us(100);
	xSemaphoreGive(spi_mux);
	return HAL_OK;
}
