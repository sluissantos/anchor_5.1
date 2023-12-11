#pragma once
#include "esp_system.h"
void ota_init(struct fw_update_t * fw_update);

#define UPDATE_ABORTED -2
#define	UPDATE_NOT_SCHEDULED 0
#define UPDATE_SCHEDULED 1
#define UPDATE_SUCCESS 2

enum protocol_type{
	HTTP,
	HTTPS
};

//enum http_auth{
//	HTTP_AUTH_TYPE_NONE,
//	HTTP_AUTH_TYPE_BASIC,
//	HTTP_AUTH_TYPE_DIGEST
//};
