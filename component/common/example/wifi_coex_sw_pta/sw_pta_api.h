/******************************************************************************
  *
  * This module is a confidential and proprietary property of RealTek and
  * possession or use of this module requires written permission of RealTek.
  *
  * Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved. 
  *
******************************************************************************/
#ifndef __SW_PTA_API_H__
#define __SW_PTA_API_H__
#include "wifi_constants.h"


#define DEV_FLAG_WIFI_RX_BEACON	0x40
#define DEV_FLAG_BT_ACTIVE	0x80
/**
 * @brief Stop internal devices(wifi and bt)
 *
 * @param[in]  bk1: need backup some information for resume.
 * @param[in]  bk2: need backup some information for resume.
 * @return  1: success
 * @return  0: fail
 */
char sw_pta_internal_dev_stop(u8 *bk1, u8 *bk2);

/**
 * @brief resume internal devices(wifi and bt)
 *
 * @param[in]  bk1: get from sw_pta_internal_dev_stop
 * @param[in]  bk2: get from sw_pta_internal_dev_stop
 * @return  1: success
 * @return  0: fail
 * @note  Please make sure get the backup information when call sw_pta_internal_dev_stop
 */
char sw_pta_internal_dev_resume(u8 *bk1, u8 *bk2);

/**
 * @brief get wifi and bt's important flags.
 *
 * @param[in]  dev_flag: 
 *				BIT7=1: bt is active
 *				BIT6=1: wifi rx beacon
 * @return  none
 */
void sw_pta_get_internal_dev_flag(u8 *dev_flag);

/**
 * @brief get wifi and bt's coexistence state.
 *
 * @param[in]  wifi_state: 
 *				COEX_STATE_WIFI_OFF = 0,
 *				COEX_STATE_WIFI_STA_UNCONNECTED = 0x01,
 *				COEX_STATE_WIFI_STA_SCAN = 0x02,
 *				COEX_STATE_WIFI_STA_CONNECTING = 0x03,
 *				COEX_STATE_WIFI_STA_CONNECTED=0x04,
 *				COEX_STATE_WIFI_AP_IDLE=0x05,
 *				COEX_STATE_WIFI_OTHER_MODE=0x06,
 * @param[in]  bt_state: 
 *				COEX_STATE_BT_OFF = 0,
 *				COEX_STATE_BT_ON = 0x01,
 *				COEX_STATE_BLE_SCAN =0x02,
 * @return  none
 */
void wifi_coex_get_internal_dev_state(rtw_coex_state_bt_t *wifi_state, rtw_coex_state_wifi_t *bt_state);

/**
 * @brief check is wifi in ips mode, which will shutdown wifi hardware.
 *

 * @return  1: wifi is in ips mode
 *              0: wifi is not in ips mode
 */
int sw_pta_wifi_is_ips(void);
#endif

