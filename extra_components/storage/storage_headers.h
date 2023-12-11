/**
 * @file storage_headers.h
 * @author Luis Henrique Maciel (luis@logpyx.com)
 * @author Henrique Ferreira (henrique.ferreira@logpyx.com)
 * @brief Esse arquivo apresenta as declarações das fuções globais para leitura/escrita de dados
 * dentro da partição dedicada da flash, além das keys para acesso a memória.  
 * 
 * @version 2.0.0
 * @date 2023-06-12
 * 
 * @attention
 * 
 * <h2><center>&copy; COPYRIGHT 2023 LogPyx S/A</center></h2>
 * 
 */


#ifndef STORAGE_HEADERS_H
#define STORAGE_HEADERS_H


/* Includes ------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "./../device/device_headers.h"
//#include "./../mqtts/mqtt_headers.h"
//#include "./../wifi/wifi_headers.h"
//#include "./../ota/ota_headers.h"
//#include "./../rele/rele_headers.h"
#include "nvs_flash.h"


/** @addtogroup Storage Aura Storage
  * @{
  */

	/** @defgroup STORAGE_code  Aura Storage Headers
	  * @{
	  */

		/** @defgroup STORAGE_Exported_Types Typedef Globais
		* @{
		*/

            /**
             * @brief Período de requisição da central
             * 
             */ 
            typedef enum {

                PERIOD_1300MS=0,        /*!< Período de 1300 milisegundos */

                PERIOD_650MS,           /*!< Período de 650 milisegundos */

                PERIOD_380MS            /*!< Período de 380 milisegundos */

            } period_request_t;

		/**
		* @}
		*/


		/** @defgroup STORAGE_Exported_Constants Constantes Globais
		* @{
		*/

            /** @defgroup STORAGE_Exported_Keys Keys de Acesso a Memória
		    * @{
		    */

                /** @defgroup STORAGE_Exported_Keys_Forklift Keys Forklift
                * @{
                */
                    
                    /**
                     * @brief Key para o Decawave ID do periférico 01.
                     * 
                     */
                    #define STORAGE_OR_FACTORY_DECAID_KEY0         "DECAWAVE_ID0"


                    /**
                     * @brief Key para o Decawave ID do periférico 02.
                     * 
                     */
                    #define STORAGE_OR_FACTORY_DECAID_KEY1         "DECAWAVE_ID1"


                    /**
                     * @brief Key para o Decawave ID do periférico 03.
                     * 
                     */
                    #define STORAGE_OR_FACTORY_DECAID_KEY2         "DECAWAVE_ID2"


                    /**
                     * @brief Key para a distância da zona de alarme FAR
                     * 
                     */
                    #define STORAGE_OR_FACTORY_FAR_DISTANCE        "FAR_DIST"


                    /**
                     * @brief Key para a distância da zona de alarme NEAR
                     * 
                     */
                    #define STORAGE_OR_FACTORY_NEAR_DISTANCE       "NEAR_DIST"


                    /**
                     * @brief Key para offset padrão do periférico
                     * 
                     */
                    #define FACTORY_OFFSET                          "PER_OFFSET"


                    /**
                     * @brief Key para o período de requisição da central
                     * 
                     */
                    #define STORAGE_OR_FACTORY_PERIOD              "PERIOD"


                    /**
                     * @brief Key para a distância mínima de validação da distância no acionamento do relé
                     * 
                     */
                    #define STORAGE_OR_FACTORY_MINIMUM_DIST         "MIN_DIST"


                    /**
                     * @brief Key para a distância máxima de validação da distância no acionamento do relé
                     * 
                     */
                    #define STORAGE_OR_FACTORY_MAXIMUM_DIST         "MAX_DIST"
                
                /**
                * @}
                */


                /** @defgroup STORAGE_Exported_Keys_Over_Crane Keys Over Crane
                * @{
                */
                    
                    /**
                     * @brief Key para o Decawave ID da Tag Fixa
                     * 
                     */
                    #define STORAGE_OR_FACTORY_TAG_FIX                         "TAG_FIX"


                    /**
                     * @brief Key para o offset no eixo x
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_OFFX                   "OFX"


                    /**
                     * @brief Key para o offset no eixo y
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_OFFY                   "OFY"


                    /**
                     * @brief Key para distância máxima
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_SEC_DIST_MAX           "DIST_MAX"


                    /**
                     * @brief Key para distância mínima
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_SEC_DIST_MIN           "DIST_MIN"


                    /**
                     * @brief Key para altura máxima
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_ALTURA_MAX             "ALT_MAX"


                    /**
                     * @brief Key para altura mínima
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_ALTURA_MIN             "ALT_MIN"


                    /**
                     * @brief Key para distância máxima da âncora 
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_DIST_ANCH_MAX          "DIST_ANCH_MAX"


                    /**
                     * @brief Key para distância mínima da âncora 
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OVERCRANE_DIST_ANCH_MIN          "DIST_ANCH_MIN"
                
                /**
                * @}
                */


                /** @defgroup STORAGE_Exported_Keys_Access_Control Keys Access Control
                * @{
                */

                    /**
                     * @brief Key para definir o modo constante dos relés
                     * 
                     */
                    #define STORAGE_OR_FACTORY_ACCESS_CONTROL_CONSTANTLY_STATE              "GATE_CSTY_MODE"
        

                    /**
                     * @brief Key pare definir o processo de abertura da porta
                     * 
                     */
                    #define STORAGE_OR_FACTORY_ACCESS_CONTROL_OPENING_OPERATION             "GATE_OPNG_MODE"


                    /**
                     * @brief Key pare definir o processo de fechamento da porta
                     * 
                     */
                    #define STORAGE_OR_FACTORY_ACCESS_CONTROL_CLOSURE_OPERATION             "GATE_CLSR_MODE"


                    /**
                     * @brief Key pare definir o timeout de abertura da porta
                     * 
                     */
                    #define STORAGE_OR_FACTORY_ACCESS_CONTROL_TIMEOUT_OPENING             "GATE_TIME_OPEN"


                    /**
                     * @brief Key pare definir o timeout de fechamento da porta
                     * 
                     */
                    #define STORAGE_OR_FACTORY_ACCESS_CONTROL_TIMEOUT_CLOSING            "GATE_TIME_CLOSE"

                /**
                * @}
                */


                /** @defgroup STORAGE_Exported_Keys_MQTT Keys MQTT/MQTTS
                * @{
                */
                    
                    /**
                     * @brief Key para o URL do broker MQTT/MQTTS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_MQTT_URL            "MQTT_URL"


                    /**
                     * @brief Key para o usuário do broker MQTT/MQTTS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_MQTT_USER           "MQTT_USER"


                    /**
                     * @brief Key para o senha do broker MQTT/MQTTS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_MQTT_PWD            "MQTT_PWD"

                /**
                * @}
                */


               /** @defgroup STORAGE_Exported_Keys_WIFI Keys WIFI
                * @{
                */
                    
                    /**
                     * @brief Key para o SSID do roteador WIFI
                     * 
                     */
                    #define STORAGE_OR_FACTORY_WIFI_SSID           "WIFI_SSID"


                    /**
                     * @brief Key para o senha do roteador WIFI
                     * 
                     */
                    #define STORAGE_OR_FACTORY_WIFI_PWD            "WIFI_PWD"


                    /**
                     * @brief Key para o tipo de autentificação da rede WIFI
                     * 
                     */
                    #define STORAGE_OR_FACTORY_WIFI_AUTH_TYPE      "WIFI_AUTH_TYPE"
                
                /**
                * @}
                */


               /** @defgroup STORAGE_Exported_Keys_OTA Keys OTA
                * @{
                */
                    
                    /**
                     * @brief Key para o usuário do cliente HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_USER            "OTA_USER"


                    /**
                     * @brief Key para a senha do cliente HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_PWD             "OTA_PWD"


                    /**
                     * @brief Key para a URL do servidor HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_URL             "OTA_URL"


                    /**
                     * @brief Key para o tipo de autentificação do servidor HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_AUTH_TYPE       "OTA_AUTH_TYPE"


                    /**
                     * @brief Key para a quatidade máxima de tentativas de conexão com o servidor HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_ATTEMPTS        "OTA_ATTEMPTS"


                    /**
                     * @brief Key para o timeout de conexão com o servidor HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_TIMEOUT         "OTA_TIMEOUT"


                    /**
                     * @brief Key para a porta de conexão com o servidor HTTP/HTTPS
                     * 
                     */
                    #define STORAGE_OR_FACTORY_OTA_PORT            "OTA_PORT"
                
                /**
                * @}
                */


               /** @defgroup STORAGE_Exported_Keys_Config Keys Configurações Internas
                * @{
                */

                    /** @defgroup STORAGE_Exported_Keys_Config_Version Keys Versionamento de Hardware e Firmware
                    * @{
                    */

                        /**
                         * @brief Versionamento do harwere do aura - Major
                         * 
                         */
                        #define FACTORY_HW_MAJOR                       "HW_MAJOR"


                        /**
                         * @brief Versionamento do hardware do aura - Minor
                         * 
                         */
                        #define FACTORY_HW_MINOR                       "HW_MINOR"


                        /**
                         * @brief  Versionamento do hardware do aura - Revisão
                         * 
                         */
                        #define FACTORY_HW_REVISION                    "HW_REV"


                        /**
                         * @brief Versionamento do firmware do aura - Major
                         * 
                         */
                        #define FACTORY_FW_MAJOR                       "FW_MAJOR"


                        /**
                         * @brief Versionamento do firmware do aura - Minor
                         * 
                         */
                        #define FACTORY_FW_MINOR                       "FW_MINOR"


                        /**
                         * @brief  Versionamento do firmware do aura - Revisão
                         * 
                         */
                        #define FACTORY_FW_REVISION                    "FW_REV"

                    /**
                    * @}
                    */


                    /** @defgroup STORAGE_Exported_Keys_Config_Hardware Keys Pinos do Circuito
                    * @{
                    */

                        #define FACTORY_RL_DEFAULT_STATE           "HW_RL_STATE"

                        #define FACTORY_RL_ON_OFF_CONFIG           "HW_RL_ON_OFF"


                        /**
                         * @brief Configuração da Central Aura - Modo de Operação (Tasks e Módulos ESP)
                         * 
                         */
                        #define FACTORY_AURA_CONFG                      "AURA_CONFIG"


                        /**
                         * @brief Pino para o relé A
                         * 
                         */
                        #define FACTORY_RLA_PIN                         "HW_RLA_PIN"


                        /**
                         * @brief Pino para o relé B
                         * 
                         */
                        #define FACTORY_RLB_PIN                         "HW_RLB_PIN"


                        /**
                         * @brief Pino para o relé C
                         * 
                         */
                        #define FACTORY_RLC_PIN                         "HW_RLC_PIN"


                        /**
                         * @brief Pino para o LED de action
                         * 
                         */
                        #define FACTORY_ACTLED_PIN                      "HW_ACT_LED_PIN"                    


                        /**
                         * @brief Pino para o LED vermelho
                         * 
                         */
                        #define FACTORY_REDLED_PIN                      "HW_RED_LED_PIN"


                        /**
                         * @brief Pino para o LED azul
                         * 
                         */
                        #define FACTORY_BLELED_PIN                      "HW_BLUE_BLE_PIN"


                        /**
                         * @brief Pino para o Chip Selector do módulo UWB
                         * 
                         */
                        #define FACTORY_CSUWB_PIN                       "HW_CS_UWB_PIN"
                        
                        
                        /**
                         * @brief Pino para o RESET do módulo UWB
                         * 
                         */
                        #define FACTORY_RSTUWB_PIN                       "HW_RST_UWB_PIN"


                        /**
                         * @brief Pino de RX da UART 2
                         * 
                         */
                        #define FACTORY_UART2_PIN_RX                    "UART2_PIN_RX"


                        /**
                         * @brief Pino de TX da UART 2
                         * 
                         */
                        #define FACTORY_UART2_PIN_TX                    "UART2_PIN_TX"


                        /**
                         * @brief Pino de RX da UART 0
                         * 
                         */
                        #define FACTORY_UART0_PIN_RX                    "UART0_PIN_RX"


                        /**
                         * @brief Pino de TX da UART 0
                         * 
                         */
                        #define FACTORY_UART0_PIN_TX                    "UART0_PIN_TX"

                    /**
                    * @}
                    */


                   /** @defgroup STORAGE_Exported_Keys_Config_Project Keys Definição do Projeto
                    * @{
                    */

                        /**
                         * @brief Configuração interna para o fabricante
                         * 
                         */
                        #define FACTORY_MANUFACTORY                    "MANUFACTURER"


                        /**
                         * @brief Configuração interna para o cliente
                         * 
                         */
                        #define STORAGE_OR_FACTORY_COSTUMER_NAME       "COSTUMER"


                        /**
                         * @brief Configuração interna para o modo de operação da central
                         * 
                         */
                        #define STORAGE_OR_FACTORY_OPERATION_MODE      "OPERATION_MODE"

                    /**
                    * @}
                    */


                    /** @defgroup STORAGE_Exported_Keys_Config_Partitions Keys Partições da NVS
                    * @{
                    */

                        /**
                         * @brief Configuração para partição dos parâmetros de configuração
                         * 
                         */
                        #define NVS_FACTORY_PARTITION                  "nvs_factory"


                        /**
                         * @brief Configuração para o namespace da tabela dos parâmetros de configuração
                         * 
                         */
                        #define NVS_FACTORY_NAMESPACE                  "logpyx"


                        /**
                         * @brief Configuração para partição NVS (Non-volatile Storage) do ESP
                         * 
                         */
                        #define NVS_STORAGE_PARTITION                  "nvs"


            void initStorage(void);
            void storageGetString(char* key, char* data);
            void storageGetUint8(char* nvs_key, uint8_t* int_data);
            void storageSetUint8(char* nvs_key, uint8_t* int_data);
            void storageGetUint16(char* nvs_key, uint16_t* int_data);
            void storageSetUint16(char* nvs_key, uint16_t* int_data);
            void storageGetUint32(char* nvs_key, uint32_t* int_data);
            void storageSetUint32(char* nvs_key, uint32_t* int_data);



#endif
