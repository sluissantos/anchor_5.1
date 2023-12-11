/*! ------------------------------------------------------------------------------------------------------------------
 * @file    ext_api_fulltest.c
 * @brief   Decawave device configuration and control functions
 *
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include <string.h>

#include "hal.h"
#include "dwm_api.h"
#include "esp_log.h"
#include <inttypes.h>

#define TAG "DWM1001_FULLTEST"

int frst(uint8_t * spidev)
{
   int rv=0;
   int err_cnt = 0;
   int delay_ms = 2000;
   ESP_LOGI(TAG,"Factory reset.");
   dwm_factory_reset(spidev);
   ESP_LOGI(TAG,"Wait %d ms for node to reset.", delay_ms);
   HAL_Delay(delay_ms);
   err_cnt += rv;

   if (rv == RV_OK)
   {
      dwm_deinit(spidev);
      dwm_init(spidev);
   }

   ESP_LOGI(TAG,"%s %s: err_cnt = %d", (err_cnt >0)? "ERR" : "   ", __FUNCTION__, err_cnt);
   return err_cnt;
}

int test_ver(uint8_t * spidev)
{
   // ========== dwm_ver_get ==========
   int rv=0, err_cnt = 0;
 
   dwm_ver_t ver; 
   ESP_LOGI(TAG,"dwm_ver_get(&ver)");
   dwm_ver_get(spidev, &ver);
   if(rv == RV_OK)
   {
   ESP_LOGI(TAG, "\t\tver.fw.maj  = %d", ver.fw.maj);
   ESP_LOGI(TAG, "\t\tver.fw.min  = %d", ver.fw.min);
   ESP_LOGI(TAG, "\t\tver.fw.patch= %d", ver.fw.patch);
   ESP_LOGI(TAG, "\t\tver.fw.res  = %d", ver.fw.res);
   ESP_LOGI(TAG, "\t\tver.fw.var  = %d", ver.fw.var);
   ESP_LOGI(TAG, "\t\tver.cfg     = %08" PRIx32, ver.cfg);  // Utilizando PRIx32 para um inteiro de 32 bits
   ESP_LOGI(TAG, "\t\tver.hw      = %08" PRIx32, ver.hw);

   printf("\t\tver.fw.maj  = %d\n", ver.fw.maj);
   printf("\t\tver.fw.min  = %d\n", ver.fw.min);
   printf("\t\tver.fw.patch= %d\n", ver.fw.patch);
   printf("\t\tver.fw.res  = %d\n", ver.fw.res);
   printf("\t\tver.fw.var  = %d\n", ver.fw.var);
   printf("\t\tver.cfg     = %08" PRIx32 "\n", ver.cfg);
   printf("\t\tver.hw      = %08" PRIx32 "\n", ver.hw);
   }
   ESP_LOGI(TAG,"dwm_ver_get(&ver):\t\t\t%s", rv==0 ? "pass":"fail");
   err_cnt += rv; 
   
   ESP_LOGI(TAG,"%s %s: err_cnt = %d", (err_cnt >0)? "ERR" : "   ", __FUNCTION__, err_cnt);
   return err_cnt;
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void example_external_api_fulltest(uint8_t * spidev)
{   
   int err_cnt = 0;  
   
   ESP_LOGI(TAG,"Initializing...");
   dwm_init(spidev);
//   HAL_Delay(2000);
//   dwm_reset();
//   HAL_Delay(2000);
//   dwm_init();
   HAL_Delay(2000);

//   err_cnt += frst();
//   ESP_LOGI(TAG,"Done");
//   ESP_LOGI(TAG,"external_api_fulltest over interface: %s", HAL_IF_STR);

   //err_cnt += test_ver();
//   // /*========= all done, reset module ========*/
//   err_cnt += frst();
//

   dwm_cfg_anchor_t cfg_an;
      cfg_an.initiator = 0;
      cfg_an.bridge = 1;
      //cfg_an.uwb_bh_routing = DWM_UWB_BH_ROUTING_AUTO;
      cfg_an.common.enc_en = 0;
      cfg_an.common.led_en = 0;
      cfg_an.common.ble_en = 0;
      cfg_an.common.uwb_mode = DWM_UWB_MODE_ACTIVE;//DWM_UWB_MODE_PASSIVE;//DWM_UWB_MODE_ACTIVE;
      cfg_an.common.fw_update_en = 0;
      ESP_LOGI(TAG,"dwm_cfg_anchor_set(&cfg_an)");
      dwm_cfg_anchor_set(spidev,&cfg_an);
      dwm_reset(spidev);
      HAL_Delay(2000);
      dwm_init(spidev);
      HAL_Delay(2000);

   dwm_cfg_t cfg_node;
   ESP_LOGI(TAG,"dwm_cfg_get(&cfg_node):");
   dwm_cfg_get(spidev,&cfg_node);
   ESP_LOGI(TAG,"\t\tcfg_node.common.fw_update_en  = %d", cfg_node.common.fw_update_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.common.uwb_mode      = %d", cfg_node.common.uwb_mode);
	 ESP_LOGI(TAG,"\t\tcfg_node.common.ble_en        = %d", cfg_node.common.ble_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.common.led_en        = %d", cfg_node.common.led_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.common.enc_en        = %d", cfg_node.common.enc_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.loc_engine_en        = %d", cfg_node.loc_engine_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.low_power_en         = %d", cfg_node.low_power_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.stnry_en             = %d", cfg_node.stnry_en);
	 ESP_LOGI(TAG,"\t\tcfg_node.meas_mode            = %d", cfg_node.meas_mode);
	 ESP_LOGI(TAG,"\t\tcfg_node.bridge               = %d", cfg_node.bridge);
	 ESP_LOGI(TAG,"\t\tcfg_node.initiator            = %d", cfg_node.initiator);
	 ESP_LOGI(TAG,"\t\tcfg_node.mode                 = %d", cfg_node.mode);
	 //ESP_LOGI(TAG,"\t\tcfg_node.uwb_bh_routing       = %d\n", cfg_node.uwb_bh_routing);
   ESP_LOGI(TAG,"err_cnt = %d ", err_cnt);

   uint16_t panid_origin;

   dwm_panid_get(spidev,&panid_origin);
   ESP_LOGI(TAG,"\n\t\tpanid_origin   = 0x%04x\n", panid_origin);

   if(panid_origin!=0xaaaa) {
	   uint16_t panid_set = 0xaaaa;
	   dwm_panid_set(spidev,panid_set);
	   dwm_reset(spidev);
	   esp_restart();
   }

}

void dwm1001_test_init(void)
{   
	uint8_t spidev = 0;
   example_external_api_fulltest(&spidev);
}

