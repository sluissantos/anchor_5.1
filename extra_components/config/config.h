#pragma once
#include "esp_system.h"
#include <inttypes.h>
#include "esp_err.h"

#define MAC2STRCAP "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC2STR(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]

#define NVS_FACTORY_PARTITION "nvs_factory"
#define NVS_FACTORY_NAMESPACE "logpyx"

#define HWV1_2
//#define HWV1_0

#define PAIRING_RESET_BUTTON    34
#define RED_LED_PIN 25
#define	BLUE_LED_PIN 27


#ifdef HWV1_2
#define AURA1_RST_PIN 2
#elif HWV1_0
#define AURA1_RST_PIN 12 //WRONG
#endif

#define AURA1_CS_PIN 32
#define AURA1_CS_PIN_MASK GPIO_NUM_32
#define AURA1_READY_PIN 33
#define AURA1_READY_PIN_MASK GPIO_NUM_33

#ifdef HWV1_2
#define AURA2_RST_PIN 26
#elif HWV1_0
#define AURA2_RST_PIN 35 //WRONG
#endif

#define AURA2_CS_PIN 5
#define AURA2_CS_PIN_MASK GPIO_NUM_5
#define AURA2_READY_PIN 13
#define AURA2_READY_PIN_MASK GPIO_NUM_13

#define AURA_MISO_PIN 19
#define AURA_MOSI_PIN 23
#define AURA_CLK_PIN 18

#define SPI3_CS_PIN 15
#define READY3_PIN 14

#define RELAY_PIN 4

#define DEFAULT_SSID "tecnologia"
#define DEFAULT_PASSWORD "128Parsecs!"
#define DEFAULT_BROKER_URI "mqtt://gwqa.revolog.com.br:1883"
#define DEFAULT_BROKER_USER " "
#define DEFAULT_BROKER_PWD " "

#define NORMAL_FIRMWARE 0
#define DEBUG_FIRMWARE 1

#define FIRMWARE_TYPE NORMAL_FIRMWARE

#define CONFIG_PRODUCT_ID 1
#define HOSTNAME_PREFIX "logpyx-aura"
#define PRODUCT_NAME "AURA-ANC"
#define HOSTNAME_PREFIX_WIFI HOSTNAME_PREFIX
#define HOSTNAME_BLE_PREFIX "Lpx"

//typedef struct {
//    uint32_t magic_word;        /*!< Magic word ESP_APP_DESC_MAGIC_WORD */
//    uint32_t secure_version;    /*!< Secure version */
//    uint32_t reserv1[2];        /*!< reserv1 */
//    char version[32];           /*!< Application version */
//    char project_name[32];      /*!< Project name */
//    char time[16];              /*!< Compile time */
//    char date[16];              /*!< Compile date*/
//    char idf_ver[32];           /*!< Version IDF */
//    uint8_t app_elf_sha256[32]; /*!< sha256 of elf file */
//    uint32_t reserv2[20];       /*!< reserv2 */
//} esp_app_desc_t;

struct factory_config_t {
	esp_err_t mem_read_err;
	char manufacturer[32];
    uint16_t id_plat;
    char plat_name[32];
    uint8_t hw_major;
	uint8_t hw_minor;
	uint8_t hw_rev;
    uint8_t fw_base_major;
    uint8_t fw_base_minor;
    uint8_t fw_base_rev;

    uint8_t test_ok;
    uint32_t wty_datetime;
    uint8_t mac[6];
    char qr_code[64];
    uint16_t id_factory;
    char prod_by[32];
    char mfg_batch[32];
    char mfg_local[32];
    uint32_t mfg_datetime;
    char op_name[64];
    uint8_t fact_new;
};

struct fw_update_t {
	char url[256];
	uint8_t protocol;
	uint8_t attempts;
	uint16_t timeout;
	int ret;
	bool version_not_defined;
	uint8_t http_auth_type;
	char http_user[32];
	char http_pass[64];
};

struct nvs_data_t {
	uint32_t cfg_holder;
	uint16_t product_id; //atual
	struct fw_update_t fw_update_data;
	char timezone[64];
};

struct nvs_data_t getNvsData(void);
struct factory_config_t getFactoryConfig(void);
void reparing_wifi(void);
esp_err_t config_load(void);
esp_err_t factory_reset(void);
esp_err_t config_hostname(void);
esp_err_t factory_init_data(void);
esp_err_t config_base_mac(void);

