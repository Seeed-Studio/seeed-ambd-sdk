#include "log_service.h"
#include "atcmd_bt.h"
#include <platform/platform_stdlib.h>
#include <platform_opts.h>
#if defined(CONFIG_PLATFORM_8710C)
#include <platform_opts_bt.h>
#endif

#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
extern int ble_central_at_cmd_connect(int argc, char **argv);
void fATBC (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	if(arg){
		argc = parse_param(arg, argv);
	}

	if ( argc != 3){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}
	
	ble_central_at_cmd_connect(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBC] Connect to remote device: ATBC=P/R,BLE_BD_ADDR");
	AT_PRINTK("[ATBC] P=public, R=random");
	AT_PRINTK("[ATBS] eg:ATBC=P,001122334455");
}

extern int ble_central_at_cmd_disconnect(int argc, char **argv);
void fATBD (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}

	if ( argc != 2){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_disconnect(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBD] Disconnect to remote device: ATBD=connect_id");
	AT_PRINTK("[ATBD] eg:ATBD=0");
}

extern int ble_central_at_cmd_get_conn_info(int argc, char **argv);
void fATBI (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}
	
    if (argc != 0){	
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
    }
	
	ble_central_at_cmd_get_conn_info(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBI] Get all connected device information, ATBI");
	AT_PRINTK("[ATBI] eg:ATBI");
}

extern int ble_central_at_cmd_start_advertising(int argc, char **argv);
void fATBA (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}

    if (argc != 0){	
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
    }
	
	ble_central_at_cmd_start_advertising(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBA] Start advertising, ATBA");
	AT_PRINTK("[ATBA] eg:ATBA");
}

extern int ble_central_at_cmd_get(int argc, char **argv) ;
void fATBG(void* arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}

	if(argc < 3){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_get(argc, argv);
	return;

exit:	
	AT_PRINTK("[ATBG] Get all services: ATBG=ALL,connect_id");
	AT_PRINTK("[ATBG] Discover services by uuid: ATBG=SRV,connect_id,uuid_type,uuid");
	AT_PRINTK("[ATBG] Discover characteristic: ATBG=CHARDIS,connect_id,start_handle,end_handle");
	AT_PRINTK("[ATBG] Discover characteristic by uuid: ATBG=CHARUUID,connect_id,start_handle,end_handle, type, uuid");
	AT_PRINTK("[ATBG] Discover characteristic descriptor: ATBG=CHARDDIS,connect_id,start_handle,end_handle");	
	AT_PRINTK("[ATBG] eg:ATBG=ALL,0");
	AT_PRINTK("[ATBG] eg(uuid16):ATBG=SRV,0,0,1803");
	AT_PRINTK("[ATBG] eg(uuid128):ATBG=SRV,0,1,00112233445566778899aabbccddeeff");
	AT_PRINTK("[ATBG] eg:ATBG=CHARDIS,0,0x8,0xFF");
	AT_PRINTK("[ATBG] eg(uuid16):ATBG=CHARUUID,0,0x1,0xFFFF,0,B001");
	AT_PRINTK("[ATBG] eg(uuid128):ATBG=CHARUUID,0,0x1,0xFFFF,1,00112233445566778899aabbccddeeff");
	AT_PRINTK("[ATBG] eg:ATBG=CHARDDIS,0,0xe,0x10"); 

}

extern int ble_central_at_cmd_scan(int argc, char **argv);
void fATBS(void * arg)
{
	int argc = 0;
	//int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
	}

	if((argc != 0) && (argc != 2) && (argc != 3) && (argc !=4)){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_scan(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBS] Scan:ATBS=scan_enable,filter_policy,filter_duplicate");
	AT_PRINTK("[ATBS] [scan_enable]=0-(start scan),1(stop scan)");
	AT_PRINTK("[ATBS] [filter_policy]: 0-(any), 1-(whitelist), 2-(any RPA), 3-(whitelist RPA)");
	AT_PRINTK("[ATBS] [filter_duplicate]: 0-(disable), 1-(enable)");
	AT_PRINTK("[ATBS] eg:ATBS=1");
	AT_PRINTK("[ATBS] eg:ATBS=1,0");
	AT_PRINTK("[ATBS] eg:ATBS=1,0,1");
	AT_PRINTK("[ATBS] eg:ATBS=0");
}

extern int ble_central_at_cmd_auth(int argc, char **argv);
void fATBK(void * arg)
{
	int argc = 0;
	//int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
	}

	if((argc != 3) && (argc != 4) && (argc != 6)){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_auth(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBK] Config and Set authentication information");
	AT_PRINTK("[ATBK] ATBK=SEND,conn_id");
	AT_PRINTK("[ATBK] ATBK=KEY,conn_id,passcode");
	AT_PRINTK("[ATBK] ATBK=MODE,auth_flags,io_cap,sec_enable,oob_enable");
	AT_PRINTK("[ATBK] eg:ATBK=SEND,0");
	AT_PRINTK("[ATBK] eg:ATBK=KEY,0,123456");
	AT_PRINTK("[ATBK] eg:ATBK=MODE,0x5,2,1,0");
}

extern int ble_central_at_cmd_send_userconf(int argc, char **argv);
void fATBY(void * arg)
{
	int argc = 0;
	//int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
	}

	if(argc != 3){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}
		
	ble_central_at_cmd_send_userconf(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBY] Send user confirmation when show GAP_MSG_LE_BOND_USER_CONFIRMATION");
	AT_PRINTK("[ATBY] ATBY=[conn_id],[conf]");
	AT_PRINTK("[ATBY] [conf]=0-(Reject),1(Accept)");
	AT_PRINTK("[ATBY] eg:ATBY=0,1");
}

extern int ble_central_at_cmd_read(int argc, char **argv);
void fATBR (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
	}

	if((argc != 3) && (argc != 6)){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_read(argc, argv);
	return;
exit:
	AT_PRINTK("[ATBR] Read characteristic: ATBR=conn_id,handle");
	AT_PRINTK("[ATBR] Read characterristic value by uuid: ATBR=conn_id,start_handle,end_handle,uuid_type,uuid");
	AT_PRINTK("[ATBR] eg(uuid16):ATBR=0,0xa");
	AT_PRINTK("[ATBR] eg(uuid16):ATBR=0,0x1,0xFFFF,0,B001");
	AT_PRINTK("[ATBR] eg(uuid128):ATBR=0,0x1,0xFFFF,1,00112233445566778899aabbccddeeff");
}

extern int ble_central_at_cmd_write(int argc, char **argv);
void fATBW (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}

	if(argc < 5){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}
	ble_central_at_cmd_write(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBW] Write data to service: ATBW=conn_id,type,handle,length,value");
	AT_PRINTK("[ATBW] [type]: write type: 1-(write request), 2-(write command)");
	AT_PRINTK("[ATBW] [handle]: attribute handle");
	AT_PRINTK("[ATBW] [length]: value length");
	AT_PRINTK("[ATBW] [lvalue]: overwrite the value");
	AT_PRINTK("[ATBW] eg:ATBW=0,1,0xc,0x1,02");
	AT_PRINTK("[ATBW] eg:ATBW=0,2,0x19,10");
}

extern int ble_central_at_cmd_update_conn_request(int argc, char **argv);
void fATBU (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
	}

	if(argc != 6){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_update_conn_request(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBU] Connection param update request: ATBU=conn_id,interval_min,interval_max,latency,supervision_timeout");
	AT_PRINTK("[ATBU] eg:ATBU=0,0x30,0x40,0x0,0x1F4");
}

extern int ble_central_at_cmd_bond_information(int argc, char **argv);
void fATBO (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
	}

	if(argc != 2){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}

	ble_central_at_cmd_bond_information(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBO] Clear bond information :ATBO=CLEAR");
	AT_PRINTK("[ATBO] Get bond information :ATBO=INFO");
}
#endif

#if defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG
extern int bt_config_app_init(void);
extern void bt_config_app_deinit(void);
void fATBB(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 2)
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBB]:_AT_BT_CONFIG_[ON]\n\r");
		bt_config_app_init();
	}
	else if (param == 0) {
		AT_PRINTK("[ATBB]:_AT_BT_CONFIG_[OFF]\n\r");
		bt_config_app_deinit();
	}
	else
		goto exit;
	
	return;
exit:
	AT_PRINTK("[ATBB] Start BT Config: ATBB=1");
	AT_PRINTK("[ATBB] Stop  BT Config: ATBB=0");
}
#endif
#if defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL
extern int ble_app_init(void);
extern void ble_app_deinit(void);
void fATBp(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 2)
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBc]:_AT_BLE_PERIPHERAL_[ON]\n\r");
		ble_app_init();
	}
	else if (param == 0) {
		AT_PRINTK("[ATBc]:_AT_BLE_PERIPHERAL_[OFF]\n\r");
		ble_app_deinit();
	}
	else
		goto exit;
	
	return;
exit:
	AT_PRINTK("[ATBB] Start BLE Peripheral: ATBp=1");
	AT_PRINTK("[ATBB] Stop	BLE Peripheral: ATBp=0");
}
#endif

extern void ble_app_modify_para_adv_interval(uint16_t adv_interval);
extern void ble_central_app_modify_para_scan_inerval_window(int param,uint16_t scan_param);
extern int hex_str_to_int(u32 str_len, s8*str);

void fATBa(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 3)
		goto exit;
	
	param = atoi(argv[1]);	
	if(param == 1){//modify adv interval
#if defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL
		AT_PRINTK("[ATBa]:BLE Peripheral Cmd \n");
		u16 adv_interval = atoi(argv[2]);
		ble_app_modify_para_adv_interval(adv_interval);
#endif
	}
	else if((param == 2) || (param == 3)){//modify scan interval/scan window
#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
		AT_PRINTK("[ATBa]:BLE Central Cmd \n");
		u16 scan_param = hex_str_to_int(strlen(argv[2]),(s8 *) argv[2]);
		ble_central_app_modify_para_scan_inerval_window(param,scan_param);
#endif
	}
	else 
		goto exit;

	return;
exit:
	AT_PRINTK("[ATBa] Modify adv interval: ATBa=1,adv_interval");
	AT_PRINTK("[ATBa] Modify scan interval:ATBa=2,scan_interval");
	AT_PRINTK("[ATBa] Modify scan window:ATBa=3,scan_window");
	AT_PRINTK("[ATBa] Modify adv interval: ATBa=1,1600");
	AT_PRINTK("[ATBa] Modify scan interval:ATBa=2,0x190");
	AT_PRINTK("[ATBa] Modify scan window:ATBa=3,0xC8");
}


#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
extern int ble_central_app_init(void);
extern void ble_central_app_deinit(void);
void fATBc(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 2)
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBc]:_AT_BLE_CENTRAL_[ON]\n\r");
		ble_central_app_init();
	}
	else if (param == 0) {
		AT_PRINTK("[ATBc]:_AT_BLE_CENTRAL_[OFF]\n\r");
		ble_central_app_deinit();
	}
	else
		goto exit;
	
	return;
exit:
	AT_PRINTK("[ATBB] Start BLE Central: ATBc=1");
	AT_PRINTK("[ATBB] Stop	BLE Central: ATBc=0");
}

void fATBn(void *arg)
{
	int argc = 0;
	int param = 0;
	int type = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	if ((argc != 4) && (argc != 2)){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}
	
	ble_central_at_cmd_modify_whitelist(argc, argv);

	return;
exit:
	AT_PRINTK("[ATBn] Clear white list: ATBn=0");		
	AT_PRINTK("[ATBn] Add a device to the white list: ATBn=1,P/R,addr");
	AT_PRINTK("[ATBn] Remove a device from the white list: ATBn=2,P/R,addr");
	}

#endif

#if ((defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET) && \
	(defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) && \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL))
extern int ble_scatternet_app_init(void);
extern void ble_scatternet_app_deinit(void);
void fATBf(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 2)
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBf]:_AT_BLE_Scatternet_[ON]\n\r");
		ble_scatternet_app_init();
	}
	else if (param == 0) {
		AT_PRINTK("[ATBf]:_AT_BLE_Scatternet_[OFF]\n\r");
		ble_scatternet_app_deinit();
	}
	else
		goto exit;
	
	return;
exit:
	AT_PRINTK("[ATBf] Start BLE Scatternet: ATBf=1");
	AT_PRINTK("[ATBf] Stop	BLE Scatternet: ATBf=0");
}

#if defined (CONFIG_BT_CENTRAL_CONFIG) && (CONFIG_BT_CENTRAL_CONFIG)
extern void ble_scatternet_config_at_cmd_config(void);
void fATBs(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}

    if (argc != 0){	
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
    }
	
	ble_scatternet_config_at_cmd_config();
	return;
exit:
	AT_PRINTK("[ATBf] reconfig wifi: ATBs");
}
#endif
#endif
#if defined(CONFIG_PLATFORM_8721D)
#if ((defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET) || \
	(defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL))
extern int ble_scatternet_at_cmd_set_phy(int argc, char **argv);
void fATBg(void *arg)
{
	int argc = 0;
	//int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	argc = parse_param(arg, argv);
	if(argc != 3){
		AT_PRINTK("[AT_PRINTK] ERROR: input parameter error!\n\r");
		goto exit;
	}
	ble_scatternet_at_cmd_set_phy(argc, argv);
	return;

exit:
	AT_PRINTK("[ATBg] Set BLE PHY(2M): ATBg=conn_id,phy");
	AT_PRINTK("[ATBg] Set BLE PHY(2M): ATBg=0,1");
}

#endif
#endif


#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
extern int bt_airsync_config_app_init(void);
extern void bt_airsync_config_app_deinit(void);
void fATBb(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;

	argc = parse_param(arg, argv);

	if (argc < 2)
		goto exit;

	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBb]:_AT_BT_AIRSYNC_CONFIG_[ON]\n\r");
		bt_airsync_config_app_init();
	}
	else if (param == 0) {
		AT_PRINTK("[ATBb]:_AT_BT_AIRSYNC_CONFIG_[OFF]\n\r");
		bt_airsync_config_app_deinit();
	}
	else
		goto exit;

	return;
exit:
	AT_PRINTK("[ATBb] Start BT Airsync Config: ATBb=1");
	AT_PRINTK("[ATBb] Stop  BT Airsync Config: ATBb=0");
}
#endif

#if defined(CONFIG_BT_BEACON) && CONFIG_BT_BEACON
extern int bt_beacon_app_init(int type);
extern void bt_beacon_app_deinit(void);
void fATBJ(void *arg)
{
	int argc = 0;
	int param = 0;
	int type = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	if ((argc != 2) && (argc != 3))
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1 && argc == 3) {
		type = atoi(argv[2]);
		if(type == 1)
			AT_PRINTK("[ATBJ]:_AT_BT_I_BEACON_[ON]\n\r");
		else if(type == 2)
			AT_PRINTK("[ATBJ]:_AT_BT_ALT_BEACON_[ON]\n\r");
		else
			goto exit;
		bt_beacon_app_init(type);
	} else if (param == 0 && argc == 2) {
		AT_PRINTK("[ATBJ]:_AT_BT_BEACON_[OFF]\n\r");
		bt_beacon_app_deinit();
	} else
		goto exit;

	return;
exit:
	AT_PRINTK("[ATBJ] Start BT I_Beacon: ATBJ=1,1");		
	AT_PRINTK("[ATBJ] Start BT Alt_Beacon: ATBJ=1,2");
	AT_PRINTK("[ATBJ] Stop	BT Beacon: ATBJ=0");
}
#endif


#if ((defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE) || \
	(defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO))
static void bt_mesh_set_cmd(unsigned int argc, char *argv[])
{
	unsigned int i = 0, j = 0;
	for (i = 0; i < argc; i++) {
		for (j = 0; j < (strlen(argv[i])); j++) {
			app_send_uart_msg(argv[i][j]);
		}
		if ((i + 1) == argc)
			app_send_uart_msg('\r');
		else
			app_send_uart_msg(' ');
	}
}

void fATBM(void *arg)
{
	unsigned int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}
	
	if((argc == 2) && (strcmp(argv[1], "?") == 0)){
		AT_PRINTK("provision cmd example: ATBM=pro,cmd,parameter \n");
		AT_PRINTK("device cmd example: ATBM=dev,cmd,parameter \n");
		goto exit;
	}

	if(argc < 3){
		AT_PRINTK("[ATBM]:not enough number of parameter, please use ATBM=? for help \n");
		goto exit;
	}	
	
	if(strcmp(argv[1], "pro") == 0){
		AT_PRINTK("[ATBM]:Provisioner Cmd \n");
	}else if(strcmp(argv[1], "dev") == 0){
		AT_PRINTK("[ATBM]:Device Cmd \n");
	}else{
		AT_PRINTK("[ATBM]:It must be dev or pro, please use ATBM=? to help \n");
		goto exit;
	}
	
	bt_mesh_set_cmd((argc-2), &argv[2]);

exit:
	return;
}
#endif

#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL) || \
	(defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET) || \
	(defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG)	|| \
	(defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG) || \
	(defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE) || \
	(defined(CONFIG_BT_BEACON) && CONFIG_BT_BEACON) || \
	(defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO))
log_item_t at_bt_items[ ] = {
#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
	{"ATBC", fATBC,{NULL,NULL}}, // Create a GATT connection	
	{"ATBD", fATBD,{NULL,NULL}}, // Disconnect GATT Connection
	{"ATBG", fATBG,{NULL,NULL}}, // Get peripheral information
	{"ATBI", fATBI,{NULL,NULL}}, // Get information of connected device
	{"ATBK", fATBK,{NULL,NULL}}, // Reply GAP passkey
	{"ATBS", fATBS,{NULL,NULL}}, // Scan BT
	{"ATBY", fATBY,{NULL,NULL}}, // Reply GAP user confirm
	{"ATBR", fATBR,{NULL,NULL}}, // GATT client read
	{"ATBW", fATBW,{NULL,NULL}}, // GATT client write
	{"ATBU", fATBU,{NULL,NULL}}, // Update connection request
	{"ATBO", fATBO,{NULL,NULL}}, // Get / clear bond information
	{"ATBA", fATBA,{NULL,NULL}}, // start advertising
	{"ATBc", fATBc,{NULL,NULL}},  //start/stop BLE central
	{"ATBn", fATBn,{NULL,NULL}},//modify whitelist
#endif
#if defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG
	{"ATBB", fATBB,{NULL,NULL}},
#endif
#if defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL
	{"ATBp", fATBp,{NULL,NULL}},  //start/stop BLE peripheral
#endif
	{"ATBa", fATBa,},  //modify adv interval/scan interval/scan window
#if ((defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET) && \
	(defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) && \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL))
	{"ATBf", fATBf,{NULL,NULL}}, //start/stop ble scatternet
#if defined (CONFIG_BT_CENTRAL_CONFIG) && CONFIG_BT_CENTRAL_CONFIG
	{"ATBs", fATBs,{NULL,NULL}}, // restart bt_config
#endif
#endif
#if defined(CONFIG_PLATFORM_8721D)
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL) || \
	(defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET))
	{"ATBg", fATBg,{NULL,NULL}}, //set PHY 2M
#endif
#endif
#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
	{"ATBb", fATBb,{NULL,NULL}},
#endif
#if defined(CONFIG_BT_BEACON) && CONFIG_BT_BEACON
	{"ATBJ", fATBJ,{NULL,NULL}},
#endif
#if ((defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE) || \
	(defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO))
	{"ATBM", fATBM,{NULL,NULL}},
#endif
};
#endif

void at_bt_init(void)
{
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_PERIPHERAL) && CONFIG_BT_PERIPHERAL) || \
	(defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG) || \
	(defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG) || \
	(defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE) || \
	(defined(CONFIG_BT_BEACON) && CONFIG_BT_BEACON) || \
	(defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO))
	log_service_add_table(at_bt_items, sizeof(at_bt_items)/sizeof(at_bt_items[0]));
#endif
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_bt_init);
#endif

