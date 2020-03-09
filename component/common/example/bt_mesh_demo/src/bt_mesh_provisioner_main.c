/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE scatternet project, mainly used for initialize modules
   * @author    jane
   * @date      2017-06-12
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdlib.h>
#include <os_sched.h>
#include <string.h>
#include <app_task.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap_msg.h>
#include <bte.h>
#include <gap_config.h>
#include "mesh_api.h"
#if MESH_GATT_CLIENT_SUPPORT
#include <profile_client.h>
#include <gaps_client.h>
#endif
#include <gap_adv.h>
#include <profile_server.h>
#include <gatt_builtin_services.h>
#include <platform_utils.h>

#include "mesh_cmd.h"
#include "provisioner_app.h"
#include "health.h"
#include "ping.h"
#include "ping_app.h"
#include "tp.h"
#include "generic_client_app.h"
#include "light_client_app.h"
#if MESH_GATT_CLIENT_SUPPORT
#include "provision_client.h"
#include "proxy_client.h"
#endif
#include "datatrans_client_app.h"
#include "app_flags.h"
#include "bt_mesh_app_lib_intf.h"
#include "vendor_cmd.h"
#include "vendor_cmd_bt.h"

#define COMPANY_ID        0x005D
#define PRODUCT_ID        0x0000
#define VERSION_ID        0x0000

mesh_model_info_t generic_on_off_client_model;

extern struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;

static int32_t generic_on_off_client_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    UNUSED(pmodel_info);
    switch (type)
    {
    case GENERIC_ON_OFF_CLIENT_STATUS:
        {
            generic_on_off_client_status_t *pdata = pargs;
            if (pdata->optional)
            {
                data_uart_debug("goo client receive: src = 0x%x, present = %d, target = %d, remain time = step(%d), resolution(%d)\r\n", \
                    pdata->src, pdata->present_on_off, pdata->target_on_off, pdata->remaining_time.num_steps, pdata->remaining_time.step_resolution);
            }
            else
            {
                data_uart_debug("goo client receive: src = 0x%x, present = %d\r\n", pdata->src, pdata->present_on_off);
                update_proed_dev(pdata->src, pdata->present_on_off);
                if (bt_mesh_lib_priv.connect_device_goog_sema && bt_mesh_lib_priv.connect_device_goog_mesh_addr == pdata->src) {
                    bt_mesh_lib_priv.connect_device_goog_light_state = pdata->present_on_off;
                    rtw_up_sema(&bt_mesh_lib_priv.connect_device_goog_sema);
                }
                if (bt_mesh_lib_priv.set_node_state_sema && bt_mesh_lib_priv.set_node_state_mesh_addr == pdata->src) {
                    bt_mesh_lib_priv.set_node_state_light_state = pdata->present_on_off;
                    rtw_up_sema(&bt_mesh_lib_priv.set_node_state_sema);
                }
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

void generic_on_off_client_model_init(void)
{
    generic_on_off_client_model.model_data_cb = generic_on_off_client_data;
    generic_on_off_client_reg(0, &generic_on_off_client_model);
}

void bt_mesh_example_device_info_cb(uint8_t bt_addr[6], uint8_t bt_addr_type, int8_t rssi, device_info_t *pinfo)
{
    uint8_t NULL_UUID[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (pinfo->type == DEVICE_INFO_UDB) {
        if (rtw_memcmp(pinfo->pbeacon_udb->dev_uuid, NULL_UUID, 16) == 0)
            add_unproed_dev(pinfo->pbeacon_udb->dev_uuid);
    }
}

void generic_on_off_client_subscribe(void)
{
    uint16_t sub_addr = 0xFEFF;
    mesh_model_p pmodel = generic_on_off_client_model.pmodel;

    if (pmodel == NULL || MESH_NOT_SUBSCRIBE_ADDR(sub_addr))
        return;
    else
        mesh_model_sub(pmodel, sub_addr);
}

/******************************************************************
 * @fn         Initial gap parameters
 * @brief      Initialize peripheral and gap bond manager related parameters
 *
 * @return     void
 */
void mesh_stack_init(void)
{
    /** set ble stack log level, disable nonsignificant log */
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_ERROR, 0);
    log_module_trace_set(TRACE_MODULE_SNOOP, TRACE_LEVEL_ERROR, 0);

    /** set mesh stack log level, default all on, disable the log of level LEVEL_TRACE */
    uint32_t module_bitmap[MESH_LOG_LEVEL_SIZE] = {0};
    diag_level_set(TRACE_LEVEL_TRACE, module_bitmap);

    /** mesh stack needs rand seed */
    plt_srand(platform_random(0xffffffff));

    /** set device name and appearance */
    char *dev_name = "Mesh Provisioner";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    gap_sched_params_set(GAP_SCHED_PARAMS_DEVICE_NAME, dev_name, GAP_DEVICE_NAME_LEN);
    gap_sched_params_set(GAP_SCHED_PARAMS_APPEARANCE, &appearance, sizeof(appearance));

    /** configure provisioning parameters */
    prov_params_set(PROV_PARAMS_CALLBACK_FUN, (void *)prov_cb, sizeof(prov_cb_pf));

    /** config node parameters */
    mesh_node_features_t features =
    {
        .role = MESH_ROLE_PROVISIONER,
        .relay = 1,
        .proxy = 1,
        .fn = 0,
        .lpn = 2,
        .prov = 1,
        .udb = 0,
        .snb = 1,
        .bg_scan = 1,
        .flash = 1,
        .flash_rpl = 1
    };
    mesh_node_cfg_t node_cfg =
    {
        .dev_key_num = 20,
        .net_key_num = 3,
        .app_key_num = 3,
        .vir_addr_num = 3,
        .rpl_num = 0,
        .sub_addr_num = 10,
        .proxy_num = 1
    };
    mesh_node_cfg(features, &node_cfg);
    mesh_node.net_trans_count = 5;
    mesh_node.relay_retrans_count = 5;
    mesh_node.trans_retrans_count = 7;

    /** create elements and register models */
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    cfg_client_reg();
    ping_control_reg(ping_app_ping_cb, pong_receive);
    trans_ping_pong_init(ping_app_ping_cb, pong_receive);
    tp_control_reg();
    generic_on_off_client_model_init();
    init_bt_mesh_priv();
    light_client_models_init();
    datatrans_client_model_init();
    compo_data_page0_header_t compo_data_page0_header = {COMPANY_ID, PRODUCT_ID, VERSION_ID};
    compo_data_page0_gen(&compo_data_page0_header);

    /** init mesh stack */
    mesh_init();

    /** configure provisioner */
    mesh_node.node_state = PROV_NODE;
    //mesh_node.unicast_addr = bt_addr[0] % 99 + 1;
    const uint8_t net_key[] = MESH_NET_KEY;
    const uint8_t net_key1[] = MESH_NET_KEY1;
    const uint8_t app_key[] = MESH_APP_KEY;
    const uint8_t app_key1[] = MESH_APP_KEY1;
    uint16_t net_key_index = net_key_add(0, net_key);
    app_key_add(net_key_index, 0, app_key);
    uint8_t net_key_index1 = net_key_add(1, net_key1);
    app_key_add(net_key_index1, 1, app_key1);

    /** register udb/provision adv/proxy adv callback */
    device_info_cb_reg(bt_mesh_example_device_info_cb);
    hb_init(hb_cb);
    generic_on_off_client_subscribe();
}

/**
  * @brief  Initialize gap related parameters
  * @return void
  */
void app_le_gap_init(void)
{
    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
#endif
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    le_register_app_cb(app_gap_callback);

#if F_BT_LE_5_0_SET_PHY_SUPPORT
    uint8_t  phys_prefer = GAP_PHYS_PREFER_ALL;
    uint8_t  tx_phys_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_2M_BIT |
                              GAP_PHYS_PREFER_CODED_BIT;
    uint8_t  rx_phys_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_2M_BIT |
                              GAP_PHYS_PREFER_CODED_BIT;
    le_set_gap_param(GAP_PARAM_DEFAULT_PHYS_PREFER, sizeof(phys_prefer), &phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_TX_PHYS_PREFER, sizeof(tx_phys_prefer), &tx_phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_RX_PHYS_PREFER, sizeof(rx_phys_prefer), &rx_phys_prefer);
#endif

    vendor_cmd_init(app_vendor_callback);
}

/**
 * @brief  Add GATT services, clients and register callbacks
 * @return void
 */
void app_le_profile_init(void)
{
    server_init(MESH_GATT_SERVER_COUNT);
    /* Add Server Module */

    /* Register Server Callback */
    server_register_app_cb(app_profile_callback);

#if MESH_GATT_CLIENT_SUPPORT
    client_init(MESH_GATT_CLIENT_COUNT + 3);
    /* Add Client Module */
    prov_client_add(app_client_callback);
    proxy_client_add(app_client_callback);

    /* Register Client Callback--App_ClientCallback to handle events from Profile Client layer. */
    client_register_general_client_cb(app_client_callback);
#endif
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{

}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
void driver_init(void)
{

}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void pwr_mgr_init(void)
{
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Scatternet APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    app_task_init();
}

void bt_stack_config_init(void)
{
    gap_config_max_le_link_num(APP_MAX_LINKS);
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int ble_app_main(void)
{
    bt_trace_init();
    //bt_stack_config_init();
    bte_init();
    board_init();
    driver_init();
    le_gap_init(APP_MAX_LINKS);
    app_le_gap_init();
    app_le_profile_init();
    mesh_stack_init();
    pwr_mgr_init();
    task_init();

    return 0;
}
