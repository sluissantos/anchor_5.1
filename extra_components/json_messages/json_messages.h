#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "dwm_api.h"
#include "dwm1001_main.h"
#include "cJSON.h"

int json_firmware_update_cmd_recv(char *buf);
char * json_firmware_update_return(int update_return);
char* json_dwm1001_alive_on(struct dmw1001_status_t * dmw1001_status_info, uint8_t spidev);
char* json_dwm1001_alive_off(void);
void json_cmd_config_uwb(char *buf, bool is_broadcast);
void json_cmd_config_wifi(char *buf);
