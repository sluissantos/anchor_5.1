#pragma once
#include "esp_system.h"
#include "dwm_api.h"
void dwm1001_init(void);
bool check_dwm1001_status(uint8_t spidev);

#define NUM_OF_DWM1001_MODULES 1
#define DWM1001_SPIDEV_ID_NOT_FOUND 0xFF

struct dmw1001_status_t {
	uint64_t node_id;
	bool alive;
	uint8_t label[DWM_LABEL_LEN_MAX];
	uint16_t panid;
	dwm_cfg_t cfg_node;
	dwm_pos_t dev_pos;
};


uint8_t dwm1001_search_device_by_id(char * node_id_search);
