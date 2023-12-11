/**
 * @file storage_main.c
 * @author Luis Henrique Maciel (luis@logpyx.com)
 * @author Henrique Ferreira (henrique.ferreira@logpyx.com)
 * @brief Esse arquivo apresenta as funções para leitura e escrita nas partições de armazenamento
 * para realizar tanto a configuração geral do Aura quanto das variáveis internas.  
 * 
 * @version 2.0.0
 * @date 2023-06-13
 * 
 * @attention
 * 
 * <h2><center>&copy; COPYRIGHT 2023 LogPyx S/A</center></h2>
 * 
 */


/* Includes ------------------------------------------------------------*/
#include "storage_headers.h"
//#include "./../rele/rele_headers.h"
//#include "./../mqtts/mqtt_headers.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"


nvs_handle_t storageHandler;

nvs_handle_t storageFCHandler;

nvs_handle_t storageACHandler;

size_t required_size;

static const char *TAG = "STORAGE";

/**
 * @brief Abertura do handler na partição de armazenamento
 * 
 * @param  None
 * @retval None
 */
void storageOpen(void){
esp_err_t err = nvs_open(NVS_STORAGE_PARTITION, NVS_READWRITE, &storageHandler);
ESP_ERROR_CHECK(err);
}


/**
 * @brief Fechamento do handler na partição de armazenamento
 * 
 * @param  None
 * @retval None
 */
void storageCloseHandler(void){
nvs_close(storageHandler);
}


/**
 * @brief Abertura do handler na partição de configurações de fábrica 
 * 
 * @param  None
 * @retval None
 */
void storageFactoryOpen(void){
    esp_err_t err = nvs_open_from_partition(NVS_FACTORY_PARTITION, NVS_FACTORY_NAMESPACE, NVS_READONLY, &storageFCHandler);
    ESP_ERROR_CHECK(err);
}


/**
 * @brief Fechamento do handler na partição de configurações de fábrica 
 * 
 * @param  None
 * @retval None
 */
void storageCloseFCHandler(void){
    nvs_close(storageFCHandler);
}



/**
 * @brief Realiza a verificação de erro da instrução de acesso a NVS e faz
 *        o commit do comando de escrita 
 * 
 * @param err flag de erro realicionada com o comando de acesso realizado anteriormente 
 * @param storageHandler handler da NVS utilizado no comando de acesso
 * @retval None
 */
void storageCheckErrorAndCommit(esp_err_t err, nvs_handle_t handler){
    ESP_ERROR_CHECK(err);
    err = nvs_commit(handler);
    ESP_ERROR_CHECK(err);
    nvs_close(handler);
}


/**
 * @brief Realiza o processo de leitura na NVS para uma variável do tipo uint32
 * 
 * @note Caso o dado buscado na partição de storage não seja encontrado, é lido o
 * valor da variável correspondente dentro das configurações de fábrica
 * 
 * @param nvs_key label que define o local em que o dado deve ser buscado 
 * @param int_data valor que foi encontrada dentro da NVS
 * @retval None
 */
void storageGetUint32(char* nvs_key, uint32_t* int_data){
    storageOpen();
    esp_err_t err = nvs_get_u32(storageHandler, nvs_key, int_data);
    if(err != ESP_OK){
        storageFactoryOpen();
        err = nvs_get_u32(storageFCHandler, nvs_key, int_data);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        storageCloseFCHandler();
    }
    storageCloseHandler();
    ESP_LOGI(TAG,"%s(r) %lu", nvs_key, *int_data);
}

/**
 * @brief Realiza o processo de escrita na NVS para uma variável do tipo uint32
 * 
 * @param nvs_key label que define o local em que o dado deve ser salvo na NVS
 * @param int_data valor que será salvo na NVS
 * @retval None
 */
void storageSetUint32(char* nvs_key, uint32_t* int_data)
{
    storageOpen();
    esp_err_t err = nvs_set_u32(storageHandler, nvs_key, *int_data);
    storageCheckErrorAndCommit(err, storageHandler);
    ESP_LOGI(TAG,"%s(w) %lu", nvs_key, *int_data);
}


/**
 * @brief Realiza o processo de leitura na NVS para uma variável do tipo uint16
 * 
 * @note Caso o dado buscado na partição de storage não seja encontrado, é lido o
 * valor da variável correspondente dentro das configurações de fábrica
 * 
 * @param nvs_key label que define o local em que o dado deve ser buscado 
 * @param int_data valor que foi encontrada dentro da NVS
 * @retval None
 */
void storageGetUint16(char* nvs_key, uint16_t* int_data)
{
    storageOpen();
    esp_err_t err = nvs_get_u16(storageHandler, nvs_key, int_data);
    if(err != ESP_OK){
        storageFactoryOpen();
        err = nvs_get_u16(storageFCHandler, nvs_key, int_data);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        storageCloseFCHandler();
    }
    storageCloseHandler();
    ESP_LOGI(TAG,"%s(r) %u", nvs_key, *int_data);
}

/**
 * @brief Realiza o processo de escrita na NVS para uma variável do tipo uint16
 * 
 * @param nvs_key label que define o local em que o dado deve ser salvo na NVS
 * @param int_data valor que será salvo na NVS
 * @retval None
 */
void storageSetUint16(char* nvs_key, uint16_t* int_data)
{
    storageOpen();
    esp_err_t err = nvs_set_u16(storageHandler, nvs_key, *int_data);
    storageCheckErrorAndCommit(err, storageHandler);
    ESP_LOGI(TAG,"%s(w) %u", nvs_key, *int_data);
}


/**
 * @brief Realiza o processo de leitura na NVS para uma variável do tipo uint8
 * 
 * @note Caso o dado buscado na partição de storage não seja encontrado, é lido o
 * valor da variável correspondente dentro das configurações de fábrica
 * 
 * @param nvs_key label que define o local em que o dado deve ser buscado 
 * @param int_data valor que foi encontrada dentro da NVS
 * @retval None
 */
void storageGetUint8(char* nvs_key, uint8_t* int_data){
    storageOpen();
    esp_err_t err = nvs_get_u8(storageHandler, nvs_key, int_data);
    if(err != ESP_OK){
        storageFactoryOpen();
        err = nvs_get_u8(storageFCHandler, nvs_key, int_data);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        storageCloseFCHandler();
    }
    storageCloseHandler();
    ESP_LOGI(TAG,"%s(r) %u", nvs_key, *int_data);
}

/**
 * @brief Realiza o processo de escrita na NVS para uma variável do tipo uint8
 * 
 * @param nvs_key label que define o local em que o dado deve ser salvo na NVS
 * @param int_data valor que será salvo na NVS
 * @retval None
 */
void storageSetUint8(char* nvs_key, uint8_t* int_data){
    storageOpen();
    esp_err_t err = nvs_set_u8(storageHandler, nvs_key, *int_data);
    storageCheckErrorAndCommit(err, storageHandler);
    ESP_LOGI(TAG,"%s(w) %u", nvs_key, *int_data);
}


/**
 * @brief Realiza o processo de escrita na NVS para uma variável do tipo string 
 * 
 * @param storageHandler handler da NVS 
 * @param key label que define o local em que o dado deve ser salvo na NVS 
 * @param data string que será salva na NVS 
 * @retval None
 */
void storageSetString(char* key, char* data){
    storageOpen();
    esp_err_t err;
    err = nvs_set_str(storageHandler,key,data);
    storageCheckErrorAndCommit(err, storageHandler);
    ESP_LOGI(TAG,"%s(w) %s", key, data);
}

/**
 * @brief  Realiza o processo de leitura na NVS para uma variável do tipo string
 * 
 * @note Caso o dado buscado na partição de storage não seja encontrado, é lido o
 * valor da variável correspondente dentro das configurações de fábrica
 * 
 * @param storageHandler handler da NVS 
 * @param key label que define o local em que o dado deve ser buscado 
 * @param data string que foi encontrada dentro da NVS 
 * @retval None
 */
void storageGetString(char* key, char* data){
    storageOpen();
    size_t required_size;
    esp_err_t err = nvs_get_str(storageHandler,key,NULL,&required_size);

    if(err != ESP_OK) { // pairing with factory configuration
        storageFactoryOpen();

        err = nvs_get_str(storageFCHandler,key,NULL,&required_size);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);

        err = nvs_get_str(storageFCHandler,key,data,&required_size);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        
        storageCloseFCHandler();
    } else {
        
        err = nvs_get_str(storageHandler,key,data,&required_size);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
    }
    ESP_LOGI(TAG,"%s(r) %s", key, data);
    storageCloseHandler();
}



/**
 * @brief Inicializa o módulo NVS para armazenamento
 * 
 * @param  None
 * @retval None
 */
void initStorage(void){
    ESP_LOGI(TAG, "Initializing...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ret = nvs_flash_init_partition(NVS_FACTORY_PARTITION);
    if ( ret != ESP_OK) {
        ESP_LOGE(TAG, "Error init nvs factory partition (err %d)", ret);
    }

    /* nvs_stats_t nvs_stats;
    nvs_get_stats(NVS_FACTORY_PARTITION, &nvs_stats);
    ESP_LOGI(TAG, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n", 
    nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries); */
}