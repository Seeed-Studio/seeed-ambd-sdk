/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     provisioner_cmd.c
  * @brief    Source file for provisioner cmd.
  * @details  User command interfaces.
  * @author   bill
  * @date     2017-3-31
  * @version  v1.0
  * *************************************************************************************
  */

#include <string.h>
#include "trace.h"
#include "gap_wrapper.h"
#include "provisioner_cmd.h"
#include "provisioner_app.h"
#include "mesh_api.h"
#if MESH_GATT_CLIENT_SUPPORT
#include "provision_client.h"
#endif
#include "provision_provisioner.h"
#include "mesh_cmd.h"
#include "test_cmd.h"
#include "client_cmd.h"
#include "generic_client_app.h"
#include "datatrans_model.h"
#include "datatrans_client_app.h"


#if MESH_GATT_CLIENT_SUPPORT
static user_cmd_parse_result_t user_cmd_prov_discover(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("Prov Start Discover\r\n");
    prov_client_conn_id = pparse_value->dw_parameter[0];
    prov_client_start_discovery(prov_client_conn_id);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_prov_read_char(user_cmd_parse_value_t *pparse_value)
{
    /* Indicate which char to be read. */
    prov_read_type_t read_char_type = (prov_read_type_t)pparse_value->dw_parameter[0];
    /* Read by handle or UUID, 1--by UUID, 0--by handle. */
    uint8_t read_pattern = (uint8_t)pparse_value->dw_parameter[1];
    data_uart_debug("Pro Read Char\r\n");
    if (read_pattern)
    {
        prov_client_read_by_uuid(prov_client_conn_id, read_char_type);
    }
    else
    {
        prov_client_read_by_handle(prov_client_conn_id, read_char_type);
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_prov_cccd_operate(user_cmd_parse_value_t *pparse_value)
{
    /* Indicate which char CCCD command. */
    uint8_t cmd_type = (uint8_t)pparse_value->dw_parameter[0];
    /* Enable or disable, 1--enable, 0--disable. */
    bool cmd_data = (bool)pparse_value->dw_parameter[1];
    data_uart_debug("Prov Cccd Operate\r\n");
    switch (cmd_type)
    {
    case 0:/* V3 Notify char notif enable/disable. */
        {
            prov_client_data_out_cccd_set(prov_client_conn_id, cmd_data);
            proxy_ctx_set_link(prov_proxy_ctx_id, prov_client_conn_id);
        }
        break;
    default:
        return USER_CMD_RESULT_WRONG_PARAMETER;
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_prov_list(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("Prov Server Handle List:\r\nidx\thandle\r\n");
    for (prov_handle_type_t hdl_idx = HDL_PROV_SRV_START; hdl_idx < HDL_PROV_CACHE_LEN; hdl_idx++)
    {
        data_uart_debug("%d\t0x%x\r\n", hdl_idx, prov_client_handle_get(prov_client_conn_id, hdl_idx));
    }
    return USER_CMD_RESULT_OK;
}
#endif

static user_cmd_parse_result_t user_cmd_pb_adv_con(user_cmd_parse_value_t *pparse_value)
{
    uint8_t dev_uuid[16];
    plt_hex_to_bin(dev_uuid, (uint8_t *)pparse_value->pparameter[0], sizeof(dev_uuid));
    if (!pb_adv_link_open(0, dev_uuid))
    {
        data_uart_debug("PB_ADV: Link Busy!\r\n");
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_pb_adv_disc(user_cmd_parse_value_t *pparse_value)
{
    if (pb_adv_link_close(0, PB_ADV_LINK_CLOSE_SUCCESS))
    {
        data_uart_debug("PB_ADV: Link Closed!\r\n");
    }
    else
    {
        data_uart_debug("PB_ADV: Link Closed Already!\r\n");
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_prov(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("provision...\r\n");
    uint32_t attn_dur = pparse_value->dw_parameter[0];
    prov_manual = pparse_value->dw_parameter[1];
    prov_start_time = plt_time_read_ms();
    return prov_invite(attn_dur) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
}

static user_cmd_parse_result_t user_cmd_prov_stop(user_cmd_parse_value_t *pparse_value)
{
    return prov_reject() ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
}

static user_cmd_parse_result_t user_cmd_prov_auth_path(user_cmd_parse_value_t *pparse_value)
{
    prov_start_algorithm_t algo = (prov_start_algorithm_t)pparse_value->dw_parameter[0];
    prov_start_public_key_t public_key = (prov_start_public_key_t)pparse_value->dw_parameter[1];
    prov_auth_method_t auth_method = (prov_auth_method_t)pparse_value->dw_parameter[2];
    uint8_t oob_action = pparse_value->dw_parameter[3];
    uint8_t oob_size = pparse_value->dw_parameter[4];

    prov_start_t prov_start;
    prov_start.algorithm = algo;
    prov_start.public_key = public_key;
    prov_start.auth_method = auth_method;
    prov_start.auth_action.oob_action = oob_action;
    prov_start.auth_size.oob_size = oob_size;
    return prov_path_choose(&prov_start) == true ? USER_CMD_RESULT_OK : USER_CMD_RESULT_WRONG_PARAMETER;
}

static user_cmd_parse_result_t user_cmd_unprov(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("Unprovision...\r\n");
#if MESH_UNPROVISIONING_SUPPORT
    prov_unprovisioning();
#endif
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_cfg_client_key_set(user_cmd_parse_value_t
                                                           *pparse_value)
{
    uint8_t key_index = pparse_value->dw_parameter[0];
    bool use_app_key = pparse_value->dw_parameter[1] ? TRUE : FALSE;
    mesh_node.features.cfg_model_use_app_key = use_app_key;
    return cfg_client_key_set(key_index) ? USER_CMD_RESULT_OK :
           USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
}

static user_cmd_parse_result_t user_cmd_compo_data_get(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t page = pparse_value->dw_parameter[1];
    if (MESH_IS_UNASSIGNED_ADDR(dst))
    {
        data_uart_debug("CDP0 len=%d, data=", mesh_node.compo_data_size[0]);
        data_uart_dump(mesh_node.compo_data[0], mesh_node.compo_data_size[0]);
    }
    else
    {
        cfg_compo_data_get(dst, page);
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_node_reset(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    if (MESH_IS_UNASSIGNED_ADDR(dst))
    {
        mesh_node_reset();
    }
    else
    {
        cfg_node_reset(dst);
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_net_key_add(user_cmd_parse_value_t *pparse_value)
{
    if (pparse_value->para_count != 2)
    {
        return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
    }
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    if (net_key_index >= mesh_node.net_key_num ||
        mesh_node.net_key_list[net_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    cfg_net_key_add(dst, mesh_node.net_key_list[net_key_index].net_key_index_g,
                    mesh_node.net_key_list[net_key_index].pnet_key[key_state_to_new_loop(
                                                                       mesh_node.net_key_list[net_key_index].key_state)]->net_key);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_net_key_update(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    if (net_key_index >= mesh_node.net_key_num ||
        mesh_node.net_key_list[net_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    if (MESH_IS_UNASSIGNED_ADDR(dst))
    {
        if (pparse_value->para_count != 3)
        {
            return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
        }
        uint8_t net_key[MESH_COMMON_KEY_SIZE];
        plt_hex_to_bin(net_key, (uint8_t *)pparse_value->pparameter[2], MESH_COMMON_KEY_SIZE);
        return net_key_update(net_key_index, mesh_node.net_key_list[net_key_index].net_key_index_g,
                              net_key) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
    }
    else
    {
        cfg_net_key_update(dst, mesh_node.net_key_list[net_key_index].net_key_index_g,
                           mesh_node.net_key_list[net_key_index].pnet_key[key_state_to_new_loop(
                                                                              mesh_node.net_key_list[net_key_index].key_state)]->net_key);
        return USER_CMD_RESULT_OK;
    }
}

static user_cmd_parse_result_t user_cmd_app_key_add(user_cmd_parse_value_t *pparse_value)
{
    if (pparse_value->para_count != 3)
    {
        return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
    }
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    uint16_t app_key_index = pparse_value->dw_parameter[2];
    if (net_key_index >= mesh_node.net_key_num ||
        mesh_node.net_key_list[net_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    if (app_key_index >= mesh_node.app_key_num ||
        mesh_node.app_key_list[app_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    cfg_app_key_add(dst, mesh_node.net_key_list[net_key_index].net_key_index_g,
                    mesh_node.app_key_list[app_key_index].app_key_index_g,
                    mesh_node.app_key_list[app_key_index].papp_key[key_state_to_new_loop(
                                                                       mesh_node.app_key_list[app_key_index].key_state)]->app_key);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_app_key_update(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    uint16_t app_key_index = pparse_value->dw_parameter[2];
    if (net_key_index >= mesh_node.net_key_num ||
        mesh_node.net_key_list[net_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    if (app_key_index >= mesh_node.app_key_num ||
        mesh_node.app_key_list[app_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    if (MESH_IS_UNASSIGNED_ADDR(dst))
    {
        if (pparse_value->para_count != 4)
        {
            return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
        }
        uint8_t app_key[MESH_COMMON_KEY_SIZE];
        plt_hex_to_bin(app_key, (uint8_t *)pparse_value->pparameter[3], MESH_COMMON_KEY_SIZE);
        return app_key_update(app_key_index, net_key_index,
                              mesh_node.app_key_list[app_key_index].app_key_index_g,
                              app_key) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
    }
    else
    {
        cfg_app_key_update(dst, mesh_node.net_key_list[net_key_index].net_key_index_g,
                           mesh_node.app_key_list[app_key_index].app_key_index_g,
                           mesh_node.app_key_list[app_key_index].papp_key[key_state_to_new_loop(
                                                                              mesh_node.app_key_list[app_key_index].key_state)]->app_key);
        return USER_CMD_RESULT_OK;
    }
}

static user_cmd_parse_result_t user_cmd_model_app_bind(user_cmd_parse_value_t *pparse_value)
{
    if (pparse_value->para_count != 4)
    {
        return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
    }
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t element_index = pparse_value->dw_parameter[1];
    uint32_t model_id = pparse_value->dw_parameter[2];
    uint16_t app_key_index = pparse_value->dw_parameter[3];
    if (app_key_index >= mesh_node.app_key_num ||
        mesh_node.app_key_list[app_key_index].key_state == MESH_KEY_STATE_INVALID)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    cfg_model_app_bind(dst, dst + element_index, mesh_node.app_key_list[app_key_index].app_key_index_g,
                       model_id);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_model_sub_add(user_cmd_parse_value_t *pparse_value)
{
    if (pparse_value->para_count != 4)
    {
        return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
    }
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t element_index = pparse_value->dw_parameter[1];
    uint32_t model_id = pparse_value->dw_parameter[2];
    uint16_t group_addr = pparse_value->dw_parameter[3];
    if (MESH_NOT_GROUP_ADDR(group_addr))
    {
        return USER_CMD_RESULT_WRONG_PARAMETER;
    }
    cfg_model_sub_add(dst, dst + element_index, false, (uint8_t *)&group_addr, model_id);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_model_sub_delete(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t element_index = pparse_value->dw_parameter[1];
    uint32_t model_id = pparse_value->dw_parameter[2];
    uint16_t group_addr = pparse_value->dw_parameter[3];
    if (pparse_value->para_count == 4)
    {
        if (MESH_NOT_GROUP_ADDR(group_addr))
        {
            return USER_CMD_RESULT_WRONG_PARAMETER;
        }
        cfg_model_sub_delete(dst, dst + element_index, false, (uint8_t *)&group_addr, model_id);
    }
    else if (pparse_value->para_count == 3)
    {
        cfg_model_sub_delete_all(dst, dst + element_index, model_id);
    }
    else
    {
        return USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS;
    }
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_key_refresh_phase_get(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    cfg_key_refresh_phase_get(dst, net_key_index_to_global(net_key_index));
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_key_refresh_phase_set(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    uint8_t phase = pparse_value->dw_parameter[2];
    cfg_key_refresh_phase_set(dst, net_key_index_to_global(net_key_index), phase);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_datatrans_write(user_cmd_parse_value_t *pparse_value)
{
    uint8_t para_count = pparse_value->para_count;
    uint8_t data[18];

    for (uint8_t i = 0; i < para_count - 3; ++i)
    {
        data[i] = pparse_value->dw_parameter[i + 1];
    }
    datatrans_write(&datatrans_client, pparse_value->dw_parameter[0],
                    pparse_value->dw_parameter[para_count - 2], para_count - 3, data,
                    pparse_value->dw_parameter[para_count - 1]);

    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_datatrans_read(user_cmd_parse_value_t *pparse_value)
{
    datatrans_read(&datatrans_client, pparse_value->dw_parameter[0],
                   pparse_value->dw_parameter[2], pparse_value->dw_parameter[1]);

    return USER_CMD_RESULT_OK;
}


/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const user_cmd_table_entry_t provisioner_cmd_table[] =
{
    // mesh common cmd
    MESH_COMMON_CMD,
    CLIENT_CMD,
    TEST_CMD,
    // provisioner cmd
    // pb-adv
    {
        "pbadvcon",
        "pbadvcon [dev uuid]\n\r",
        "create a pb-adv link with the device uuid\n\r",
        user_cmd_pb_adv_con
    },
    {
        "pbadvdisc",
        "pbadvdisc\n\r",
        "disconnect the pb-adv link\n\r",
        user_cmd_pb_adv_disc
    },
    // pb-gatt
#if MESH_GATT_CLIENT_SUPPORT
    {
        "provdis",
        "provdis [conn id]\n\r",
        "Start discovery provisioning service\n\r",
        user_cmd_prov_discover
    },
    {
        "provread",
        "provread [char] [pattern: handle/UUID16]\n\r",
        "Read all related chars by user input\n\r",
        user_cmd_prov_read_char
    },
    {
        "provcmd",
        "provcmd [char CCCD] [command: enable/disable]\n\r",
        "Provisioning notify/ind switch command\n\r",
        user_cmd_prov_cccd_operate
    },
    {
        "provls",
        "provls\n\r",
        "Provision server handle list\n\r",
        user_cmd_prov_list
    },
#endif
    // provisioner
    {
        "prov",
        "prov [attn_dur] [manual]\n\r",
        "provision a new mesh device\n\r",
        user_cmd_prov
    },
    {
        "provs",
        "provs\n\r",
        "provision stop\n\r",
        user_cmd_prov_stop
    },
    {
        "pap",
        "pap [algorithm] [pubkey] [method: nsoi] [action] [size]\n\r",
        "provision authentication path\n\r",
        user_cmd_prov_auth_path
    },
    {
        "unprov",
        "unprov\n\r",
        "unprovision the mesh device\n\r",
        user_cmd_unprov
    },
    // cfg client key set
    {
        "ccks",
        "ccks [key_index] [use_app_key]\n\r",
        "cfg client key set\n\r",
        user_cmd_cfg_client_key_set
    },
    // cfg client or local setting
    {
        "cdg",
        "cdg [dst]\n\r",
        "compo data get\n\r",
        user_cmd_compo_data_get
    },
    {
        "nr",
        "nr [dst]\n\r",
        "node reset\n\r",
        user_cmd_node_reset
    },
    {
        "nka",
        "nka [dst] [net_key_index]\n\r",
        "net key add\n\r",
        user_cmd_net_key_add
    },
    {
        "nku",
        "nku [dst] [net_key_index] [net key]\n\r",
        "net key update\n\r",
        user_cmd_net_key_update
    },
    {
        "aka",
        "aka [dst] [net_key_index] [app_key_index]\n\r",
        "app key add\n\r",
        user_cmd_app_key_add
    },
    {
        "aku",
        "aku [dst] [net_key_index] [app_key_index] [app key]\n\r",
        "app key update\n\r",
        user_cmd_app_key_update
    },
    {
        "mab",
        "mab [dst] [element index] [model_id] [app_key_index]\n\r",
        "model app bind\n\r",
        user_cmd_model_app_bind
    },
    {
        "msa",
        "msa [dst] [element index] [model_id] [group addr]\n\r",
        "model subsribe add\n\r",
        user_cmd_model_sub_add
    },
    {
        "msd",
        "msd [dst] [element index] [model_id] <group addr>\n\r",
        "model subsribe delete\n\r",
        user_cmd_model_sub_delete
    },
    {
        "krpg",
        "krpg [dst] [net key index]\n\r",
        "key refresh phase get\n\r",
        user_cmd_key_refresh_phase_get
    },
    {
        "krps",
        "krps [dst] [net key index] [phase]\n\r",
        "key refresh phase set\n\r",
        user_cmd_key_refresh_phase_set
    },
    {
        "dtw",
        "dtw [dst] [data...] [app_key_index] [ack] \n\r",
        "data transmission write data\n\r",
        user_cmd_datatrans_write
    },
    {
        "dtr",
        "dtr [dst] [len] [app_key_index]\n\r",
        "data transmission read data\n\r",
        user_cmd_datatrans_read
    },
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
