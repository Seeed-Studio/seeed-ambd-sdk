#include "platform_opts.h"
#include "bt_mesh_app_lib_intf.h"
#include "wifi_conf.h"
#include "gap_conn_le.h"
#include "mesh_api.h"
#include "generic_on_off.h"

#define CMD_REPEAT_TIMES	5
#define BT_MESH_APP_LIB_INTF_DEBUG		1

struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;

#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER) && CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER
extern bool prov_manual;
extern uint32_t prov_start_time;
extern mesh_model_info_t generic_on_off_client_model;
#endif
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE) && CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE
extern mesh_model_info_t generic_on_off_server_model;
#endif

extern void *evt_queue_handle;
extern void *io_queue_handle;

void bt_mesh_demo_send_io_msg(T_IO_MSG *p_io_msg)
{
	uint8_t event = EVENT_USER_HTTP_SERVER;

#if BT_MESH_APP_LIB_INTF_DEBUG
	printf("[BT_MESH_DEMO] Send io_msg.type = 0x%x\r\n", p_io_msg->type);
#endif

	if (os_msg_send(io_queue_handle, p_io_msg, 0) == _FALSE)
	{
		printf("[BT_MESH_DEMO] Send io_msg to io_queue_handle fail!\r\n");
	}
	else if (os_msg_send(evt_queue_handle, &event, 0) == _FALSE)
	{
		printf("[BT_MESH_DEMO] Send io_msg to evt_queue_handle fail!\r\n");
	}
}

void bt_mesh_demo_io_msg_handler(T_IO_MSG io_msg)
{
#if BT_MESH_APP_LIB_INTF_DEBUG
	printf("[BT_MESH_DEMO] Receive io_msg.type = 0x%x\r\n", io_msg.type);
#endif

	switch (io_msg.type) {
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER) && CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER
		case API_PB_ADV_LINK_OPEN:
		{
			uint8_t *dev_uuid = io_msg.u.buf;
			uint8_t ctx_index = 0;
			if (!pb_adv_link_open(ctx_index, dev_uuid)) {
				printf("[BT_MESH_DEMO] PB_ADV: Link Busy!\r\n");
			}
		}
		break;

		case API_PROV_INVITE:
		{
			uint8_t attn_dur = 0;
			prov_manual = 0;
			prov_start_time = plt_time_read_ms();
			prov_invite(attn_dur);
		}
		break;

		case API_CFG_APP_KEY_ADD:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint16_t net_key_index = 0;
			uint16_t app_key_index = 0;
			if (net_key_index >= mesh_node.net_key_num || mesh_node.net_key_list[net_key_index].key_state == MESH_KEY_STATE_INVALID) {
				printf("[BT_MESH_DEMO] aka net_key_index fail!\r\n");
			} else {
				if (app_key_index >= mesh_node.app_key_num || mesh_node.app_key_list[app_key_index].key_state == MESH_KEY_STATE_INVALID) {
					printf("[BT_MESH_DEMO] aka app_key_index fail!\r\n");
				} else {
					cfg_app_key_add(mesh_addr, mesh_node.net_key_list[net_key_index].net_key_index_g, mesh_node.app_key_list[app_key_index].app_key_index_g,
						mesh_node.app_key_list[app_key_index].papp_key[key_state_to_new_loop(mesh_node.app_key_list[app_key_index].key_state)]->app_key);
				}
			}
		}
		break;

		case API_CFG_MODEL_APP_BIND:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint8_t element_index = 0;
			uint16_t app_key_index = 0;
			uint32_t model_id = MESH_MODEL_GENERIC_ON_OFF_SERVER;
			if (app_key_index >= mesh_node.app_key_num || mesh_node.app_key_list[app_key_index].key_state == MESH_KEY_STATE_INVALID) {
				printf("[BT_MESH_DEMO] mab app_key_index fail!\r\n");
			} else {
				cfg_model_app_bind(mesh_addr, mesh_addr + element_index, mesh_node.app_key_list[app_key_index].app_key_index_g, model_id);
			}
		}
		break;

		case API_CFG_MODEL_PUB_SET:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint8_t element_index = 0;
			bool va_flag = 0;
			uint8_t pub_addr[16];
			pub_key_info_t pub_key_info = {0, 0, 0};
			uint8_t pub_ttl = 0x3F;
			pub_period_t pub_period = {0x86 & 0x3f, 0x86 >> 6};
			pub_retrans_info_t pub_retrans_info = {0, 3};
			uint32_t model_id = MESH_MODEL_GENERIC_ON_OFF_SERVER;
			LE_WORD2EXTRN(pub_addr, 0xFEFF);
			cfg_model_pub_set(mesh_addr, mesh_addr + element_index, va_flag, pub_addr, pub_key_info, pub_ttl, pub_period, pub_retrans_info, model_id);
		}
		break;

		case API_CFG_MODEL_SUB_ADD:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint16_t group_addr = io_msg.u.param;
			uint16_t *p_group_addr = &group_addr;
			uint8_t element_index = 0;
			uint32_t model_id = MESH_MODEL_GENERIC_ON_OFF_SERVER;
			if (MESH_NOT_GROUP_ADDR(group_addr)) {
				printf("[BT_MESH_DEMO] msa MESH_NOT_GROUP_ADDR fail!\r\n");
			} else {
				cfg_model_sub_add(mesh_addr, mesh_addr + element_index, 0, (uint8_t *)p_group_addr, model_id);
			}
		}
		break;
		case API_CFG_MODEL_SUB_GET:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint8_t element_index = 0;
			uint32_t model_id = MESH_MODEL_GENERIC_ON_OFF_SERVER;
			cfg_model_sub_get(mesh_addr, mesh_addr + element_index, model_id);
		}
		break;

		case API_CFG_MODEL_SUB_DELETE_ALL:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint8_t element_index = 0;
			uint32_t model_id = MESH_MODEL_GENERIC_ON_OFF_SERVER;
			cfg_model_sub_delete_all(mesh_addr, mesh_addr + element_index, model_id);
		}
		break;

		case API_CFG_NODE_RESET:
		{
			uint16_t mesh_addr = io_msg.subtype;
			cfg_node_reset(mesh_addr);
		}
		break;

		case API_GENERIC_ON_OFF_GET:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint16_t app_key_index = 0;
			generic_on_off_get(&generic_on_off_client_model, mesh_addr, app_key_index);
		}
		break;

		case API_GENERIC_ON_OFF_SET:
		{
			uint16_t mesh_addr = io_msg.subtype;
			uint8_t light_state = io_msg.u.param;
			uint16_t app_key_index = 0;
			static uint8_t tid = 0;
			bool optional = 0;
			generic_transition_time_t trans_time = {0, 0};
			uint8_t delay = 0;
			bool ack = 1;
			tid ++;
			generic_on_off_set(&generic_on_off_client_model, mesh_addr, app_key_index, (generic_on_off_t)light_state, tid, optional, trans_time, delay, ack);
		}
		break;
#endif

#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE) && CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE
		case API_GENERIC_ON_OFF_PUBLISH:
		{
			generic_on_off_t on_off = io_msg.subtype;
			mesh_model_p pmodel = generic_on_off_server_model.pmodel;
			pmodel->pub_addr = 0xFEFF;
			pmodel->pub_ttl = 0x3F;
			generic_on_off_publish(&generic_on_off_server_model, on_off);
		}
		break;
#endif

		default:
			printf("[BT_MESH_DEMO] Unknown API!\r\n");
	}
}

uint8_t bt_mesh_cmd_start_provisioner_api(void)
{
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	/*Init BT mesh provisioner*/
	ble_app_main();

	bt_coex_init();

	/*Wait BT init complete*/
	do {
		vTaskDelay(100 / portTICK_RATE_MS);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	} while (new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();

	return 1;
}

uint8_t bt_mesh_cmd_connect_device_api(uint8_t *dev_uuid, uint16_t *mesh_addr, uint8_t *light_state)
{
	T_IO_MSG io_msg;
	uint16_t mesh_address;
	uint8_t count;

	bt_mesh_lib_priv.connect_device_sema = NULL;
	bt_mesh_lib_priv.connect_device_flag = 0;
	bt_mesh_lib_priv.connect_device_mesh_addr = 0;

	rtw_init_sema(&bt_mesh_lib_priv.connect_device_sema, 0);
	if (bt_mesh_lib_priv.connect_device_sema == NULL) {
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.connect_device_sema init fail!\r\n");
		return 0;
	}

	/* pbadvcon */
	count = 0;
	do {
		io_msg.type = API_PB_ADV_LINK_OPEN;
		io_msg.u.buf = dev_uuid;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;

	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] pbadvcon down bt_mesh_lib_priv.connect_device_sema fail!\r\n");
		goto exit;
	}

	if (bt_mesh_lib_priv.connect_device_flag == 0) {
		printf("[BT_MESH_DEMO] pbadvcon fail!\r\n");
		goto exit;
	}

	/* prov */
	count = 0;
	do {
		io_msg.type = API_PROV_INVITE;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_sema, 20 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] prov down bt_mesh_lib_priv.connect_device_sema fail!\r\n");
		goto exit;
	}

	if (bt_mesh_lib_priv.connect_device_mesh_addr == 0) {
		printf("[BT_MESH_DEMO] prov fail!\r\n");
		goto exit;
	}

	mesh_address = bt_mesh_lib_priv.connect_device_mesh_addr;

	/* aka */
	count = 0;
	do {
		io_msg.type = API_CFG_APP_KEY_ADD;
		io_msg.subtype = mesh_address;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] aka down bt_mesh_lib_priv.connect_device_sema fail!\r\n");
		goto nr_exit;
	}

	if (bt_mesh_lib_priv.connect_device_flag == 0) {
		printf("[BT_MESH_DEMO] aka fail!\r\n");
		goto nr_exit;
	}

	/* mab */
	count = 0;
	do {
		io_msg.type = API_CFG_MODEL_APP_BIND;
		io_msg.subtype = mesh_address;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] mab down bt_mesh_lib_priv.connect_device_sema fail!\r\n");
		goto nr_exit;
	}

	if (bt_mesh_lib_priv.connect_device_flag == 0) {
		printf("[BT_MESH_DEMO] mab fail!\r\n");
		goto nr_exit;
	}

	/* cms */
	count = 0;
	do {
		io_msg.type = API_CFG_MODEL_PUB_SET;
		io_msg.subtype = mesh_address;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] cms down bt_mesh_lib_priv.connect_device_sema fail!\r\n");
		goto nr_exit;
	}

	if (bt_mesh_lib_priv.connect_device_flag == 0) {
		printf("[BT_MESH_DEMO] cms fail!\r\n");
		goto nr_exit;
	}

	bt_mesh_lib_priv.connect_device_goog_sema = NULL;
	bt_mesh_lib_priv.connect_device_goog_mesh_addr = mesh_address;
	bt_mesh_lib_priv.connect_device_goog_light_state = 0;

	rtw_init_sema(&bt_mesh_lib_priv.connect_device_goog_sema, 0);
	if (bt_mesh_lib_priv.connect_device_goog_sema == NULL) {
		bt_mesh_lib_priv.connect_device_goog_mesh_addr = 0;
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.connect_device_goog_sema init fail!\r\n");
		goto nr_exit;
	}

	/* goog */
	count = 0;
	do {
		io_msg.type = API_GENERIC_ON_OFF_GET;
		io_msg.subtype = mesh_address;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_goog_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		bt_mesh_lib_priv.connect_device_goog_mesh_addr = 0;
		rtw_free_sema(&bt_mesh_lib_priv.connect_device_goog_sema);
		printf("[BT_MESH_DEMO] goog down bt_mesh_lib_priv.connect_device_goog_sema fail!\r\n");
		goto nr_exit;
	}

	*mesh_addr = mesh_address;
	*light_state = bt_mesh_lib_priv.connect_device_goog_light_state;

	bt_mesh_lib_priv.connect_device_goog_mesh_addr = 0;
	bt_mesh_lib_priv.connect_device_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.connect_device_goog_sema);
	rtw_free_sema(&bt_mesh_lib_priv.connect_device_sema);
	return 1;

nr_exit:
	bt_mesh_lib_priv.connect_device_nr_sema = NULL;
	bt_mesh_lib_priv.connect_device_nr_mesh_addr = mesh_address;

	rtw_init_sema(&bt_mesh_lib_priv.connect_device_nr_sema, 0);
	if (bt_mesh_lib_priv.connect_device_nr_sema == NULL) {
		bt_mesh_lib_priv.connect_device_nr_mesh_addr = 0;
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.connect_device_nr_sema init fail!\r\n");
		goto exit;
	}

	/* nr */
	count = 0;
	do {
		io_msg.type = API_CFG_NODE_RESET;
		io_msg.subtype = mesh_address;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.connect_device_nr_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] nr down bt_mesh_lib_priv.connect_device_nr_sema fail!\r\n");
	}

	bt_mesh_lib_priv.connect_device_nr_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.connect_device_nr_sema);

exit:
	bt_mesh_lib_priv.connect_device_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.connect_device_sema);
	return 0;
}

uint8_t bt_mesh_cmd_delete_node_api(uint16_t mesh_addr)
{
	T_IO_MSG io_msg;
	uint8_t ret;
	uint8_t count;

	bt_mesh_lib_priv.delete_node_sema = NULL;
	bt_mesh_lib_priv.delete_node_mesh_addr = mesh_addr;

	rtw_init_sema(&bt_mesh_lib_priv.delete_node_sema, 0);
	if (bt_mesh_lib_priv.delete_node_sema == NULL) {
		bt_mesh_lib_priv.delete_node_mesh_addr = 0;
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.delete_node_sema init fail!\r\n");
		return 0;
	}

	/* nr */
	count = 0;
	do {
		io_msg.type = API_CFG_NODE_RESET;
		io_msg.subtype = mesh_addr;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.delete_node_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		ret = 0;
		printf("[BT_MESH_DEMO] nr down bt_mesh_lib_priv.delete_node_sema fail!\r\n");
	} else
		ret = 1;

	bt_mesh_lib_priv.delete_node_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.delete_node_sema);
	return ret;
}

uint8_t bt_mesh_cmd_set_node_state_api(uint16_t mesh_addr, uint16_t group_addr, uint8_t light_state, uint8_t flag)
{
	T_IO_MSG io_msg;
	uint8_t ret;
	uint8_t count;

	bt_mesh_lib_priv.set_node_state_sema = NULL;
	bt_mesh_lib_priv.set_node_state_mesh_addr = mesh_addr;
	bt_mesh_lib_priv.set_node_state_light_state = 0;

	rtw_init_sema(&bt_mesh_lib_priv.set_node_state_sema, 0);
	if (bt_mesh_lib_priv.set_node_state_sema == NULL) {
		bt_mesh_lib_priv.set_node_state_mesh_addr = 0;
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.set_node_state_sema init fail!\r\n");
		return 2;
	}

	/* goos */
	count = 0;
	do {
		io_msg.type = API_GENERIC_ON_OFF_SET;
		if (flag && MESH_IS_GROUP_ADDR(group_addr))
			io_msg.subtype = group_addr;
		else
			io_msg.subtype = mesh_addr;
		io_msg.u.param = light_state;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.set_node_state_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		ret = 2;
		printf("[BT_MESH_DEMO] goos down bt_mesh_lib_priv.set_node_state_sema fail!\r\n");
	} else
		ret = bt_mesh_lib_priv.set_node_state_light_state;

	bt_mesh_lib_priv.set_node_state_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.set_node_state_sema);
	return ret;
}

uint8_t bt_mesh_cmd_set_node_group_api(uint16_t mesh_addr, uint16_t group_addr)
{
	T_IO_MSG io_msg;
	uint8_t count;

	bt_mesh_lib_priv.set_node_group_sema = NULL;
	bt_mesh_lib_priv.set_node_group_flag = 0;
	bt_mesh_lib_priv.set_node_group_mesh_addr = mesh_addr;

	rtw_init_sema(&bt_mesh_lib_priv.set_node_group_sema, 0);
	if (bt_mesh_lib_priv.set_node_group_sema == NULL) {
		bt_mesh_lib_priv.set_node_group_mesh_addr = 0;
		printf("[BT_MESH_DEMO] bt_mesh_lib_priv.set_node_group_sema init fail!\r\n");
		return 0;
	}

	/* msd */
	count = 0;
	do {
		io_msg.type = API_CFG_MODEL_SUB_DELETE_ALL;
		io_msg.subtype = mesh_addr;
		bt_mesh_demo_send_io_msg(&io_msg);

		if (rtw_down_timeout_sema(&bt_mesh_lib_priv.set_node_group_sema, 2 * 1000) == _FAIL)
			count ++;
		else
			break;
	} while (count < CMD_REPEAT_TIMES);

	if (count == CMD_REPEAT_TIMES) {
		printf("[BT_MESH_DEMO] msd down bt_mesh_lib_priv.set_node_group_sema fail!\r\n");
		goto exit;
	}

	if (bt_mesh_lib_priv.set_node_group_flag == 0) {
		printf("[BT_MESH_DEMO] msd fail!\r\n");
		goto exit;
	}

	if (group_addr != 0) {
		/* msa */
		count = 0;
		do {
			io_msg.type = API_CFG_MODEL_SUB_ADD;
			io_msg.subtype = mesh_addr;
			io_msg.u.param = group_addr;
			bt_mesh_demo_send_io_msg(&io_msg);

			if (rtw_down_timeout_sema(&bt_mesh_lib_priv.set_node_group_sema, 2 * 1000) == _FAIL)
				count ++;
			else
				break;
		} while (count < CMD_REPEAT_TIMES);

		if (count == CMD_REPEAT_TIMES) {
			printf("[BT_MESH_DEMO] msa down bt_mesh_lib_priv.set_node_group_sema fail!\r\n");
			goto exit;
		}

		if (bt_mesh_lib_priv.set_node_group_flag == 0) {
			printf("[BT_MESH_DEMO] msa fail!\r\n");
			goto exit;
		}
	}

	bt_mesh_lib_priv.set_node_group_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.set_node_group_sema);
	return 1;

exit:
	bt_mesh_lib_priv.set_node_group_mesh_addr = 0;
	rtw_free_sema(&bt_mesh_lib_priv.set_node_group_sema);
	return 0;
}

void bt_mesh_cmd_get_node_group_api(uint16_t mesh_addr)
{
	T_IO_MSG io_msg;

	/* cmg */
	io_msg.type = API_CFG_MODEL_SUB_GET;
	io_msg.subtype = mesh_addr;
	bt_mesh_demo_send_io_msg(&io_msg);
}

