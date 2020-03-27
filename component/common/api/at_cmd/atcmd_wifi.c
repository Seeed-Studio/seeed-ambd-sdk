#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "log_service.h"
#include "atcmd_wifi.h"
#include "osdep_service.h"
#include <lwip_netconf.h>
#include "tcpip.h"
#include <dhcp/dhcps.h>
#if CONFIG_WLAN
#include <wlan/wlan_test_inc.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#endif

#if ATCMD_VER == ATVER_2
#include "flash_api.h"
#include "device_lock.h"
#include <wlan_fast_connect/example_wlan_fast_connect.h>
#endif

#if ATCMD_VER == ATVER_2 || WIFI_LOGO_CERTIFICATION_CONFIG
#include <lwip/sockets.h>
#endif

#ifdef WIFI_PERFORMANCE_MONITOR
#include "wifi_performance_monitor.h"
#endif

#include "platform_opts.h"

/******************************************************************************/
#define	_AT_WLAN_SET_SSID_          "ATW0"
#define	_AT_WLAN_SET_PASSPHRASE_    "ATW1"
#define	_AT_WLAN_SET_KEY_ID_        "ATW2"
#define	_AT_WLAN_AP_SET_SSID_       "ATW3"
#define	_AT_WLAN_AP_SET_SEC_KEY_    "ATW4"
#define	_AT_WLAN_AP_SET_CHANNEL_    "ATW5"
#define _AT_WLAN_SET_BSSID_         "ATW6"
#define	_AT_WLAN_AP_ACTIVATE_       "ATWA"
#define _AT_WLAN_AP_STA_ACTIVATE_   "ATWB"
#define	_AT_WLAN_JOIN_NET_          "ATWC"
#define	_AT_WLAN_DISC_NET_          "ATWD"
#define	_AT_WLAN_WEB_SERVER_        "ATWE"
#define _AT_WLAN_P2P_FIND_          "ATWF"
#define _AT_WLAN_P2P_START_         "ATWG"
#define _AT_WLAN_P2P_STOP_          "ATWH"
#define _AT_WLAN_PING_TEST_         "ATWI"
#define _AT_WLAN_P2P_CONNECT_       "ATWJ"
#define _AT_WLAN_P2P_DISCONNECT_    "ATWK"
#define _AT_WLAN_SSL_CLIENT_        "ATWL"
#define _AT_WLAN_PROMISC_           "ATWM"
#define _AT_WLAN_P2P_INFO_          "ATWN"
#define _AT_WLAN_OTA_UPDATE_        "ATWO"
#define	_AT_WLAN_POWER_             "ATWP"
#define	_AT_WLAN_SIMPLE_CONFIG_     "ATWQ"
#define	_AT_WLAN_GET_RSSI_          "ATWR"
#define	_AT_WLAN_SCAN_              "ATWS"
#define _AT_WLAN_SCAN_WITH_SSID_    "ATWs"
#define _AT_WLAN_TCP_TEST_          "ATWT"
#define _AT_WLAN_UDP_TEST_          "ATWU"
#define _AT_WLAN_WPS_               "ATWW"
#define _AT_WLAN_AP_WPS_            "ATWw"
#define _AT_WLAN_AIRKISS_           "ATWX"
#define _AT_WLAN_IWPRIV_            "ATWZ"
#define	_AT_WLAN_INFO_              "ATW?"

#define	_AT_WLAN_EXTEND_POWER_MODE_        "ATXP"

#ifndef CONFIG_SSL_CLIENT
#define CONFIG_SSL_CLIENT       0
#endif
#ifndef CONFIG_OTA_UPDATE
#define CONFIG_OTA_UPDATE       0
#endif
#ifndef CONFIG_BSD_TCP
#define CONFIG_BSD_TCP	        1
#endif
#ifndef CONFIG_ENABLE_P2P
#define CONFIG_ENABLE_P2P		0
#endif
#define SCAN_WITH_SSID		0

#ifndef CONFIG_WOWLAN_SERVICE
#define CONFIG_WOWLAN_SERVICE 0
#endif

#if CONFIG_LWIP_LAYER
extern void cmd_tcp(int argc, char **argv);
extern void cmd_udp(int argc, char **argv);
extern void cmd_ping(int argc, char **argv);
extern void cmd_ssl_client(int argc, char **argv);
#endif

#if CONFIG_WLAN
extern void cmd_promisc(int argc, char **argv);
extern void cmd_update(int argc, char **argv);
extern void cmd_simple_config(int argc, char **argv);
#if defined(CONFIG_INCLUDE_DPP_CONFIG) && CONFIG_INCLUDE_DPP_CONFIG
extern void cmd_dpp(int argc, char **argv);
#endif
#if CONFIG_ENABLE_WPS
extern void cmd_wps(int argc, char **argv);
#endif

#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
extern void cmd_ap_wps(int argc, char **argv);
extern int wpas_wps_dev_config(u8 * dev_addr, u8 bregistrar);
#endif
#if CONFIG_ENABLE_P2P
extern void cmd_wifi_p2p_start(int argc, char **argv);
extern void cmd_wifi_p2p_stop(int argc, char **argv);
extern void cmd_p2p_listen(int argc, char **argv);
extern void cmd_p2p_find(int argc, char **argv);
extern void cmd_p2p_peers(int argc, char **argv);
extern void cmd_p2p_info(int argc, char **argv);
extern void cmd_p2p_disconnect(int argc, char **argv);
extern void cmd_p2p_connect(int argc, char **argv);
extern int cmd_wifi_p2p_auto_go_start(int argc, char **argv);
#endif				//CONFIG_ENABLE_P2P
#if CONFIG_AIRKISS
extern int airkiss_start(rtw_network_info_t *);
extern int airkiss_stop(void);
#endif

#if CONFIG_WOWLAN_SERVICE
extern void cmd_wowlan_service(int argc, char **argv);
#endif
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
extern void inic_c2h_wifi_info(const char *atcmd, char status);
extern void inic_c2h_msg(const char *atcmd, u8 status, char *msg, u16 msg_len);
#endif

/* fastconnect use wifi AT command. Not init_wifi_struct when log service disabled
 * static initialize all values for using fastconnect when log service disabled
 */
static rtw_network_info_t wifi = { 0 };

static rtw_ap_info_t ap = { 0 };
static uint8_t password[65] = { 0 };
static uint8_t ap_pwd_buf[65] = { 0 };

char at_string[ATSTRING_LEN];

#ifdef CONFIG_FPGA
int security = -1;
#endif

uint8_t       sta_ip[4]      = { 192, 168, 1, 80 },
	      sta_netmask[4] = {255, 255, 255, 0},
	      sta_gw[4]      = {192, 168, 1, 1};

int dhcp_mode_sta = DHCP_MODE_ENABLE, dhcp_mode_ap = DHCP_MODE_ENABLE;
uint8_t       ap_ip[4]      = { 192, 168, 43, 1 },
	      ap_netmask[4] = {255, 255, 255, 0},
	      ap_gw[4]      = {192, 168, 43, 1};

static int esp_ipstatus = ESP_IPSTAT_NO_AP;

static void atcmd_wifi_disconn_hdl(char *buf, int buf_len, int flags, void *userdata);

static _sema at_printf_sema = NULL;

unsigned at_prt_lock(void) {
	unsigned mask = 0;

	// protect the at_printf buffer
	if (__get_IPSR()) {
		// in interrupt routine
		mask = taskENTER_CRITICAL_FROM_ISR();
	} else {
		// or task environment
		rtw_down_sema(&at_printf_sema);
		// taskENTER_CRITICAL();
	}
	return mask;
}

int at_prt_unlock(unsigned mask) {
	if (__get_IPSR()) {
		taskEXIT_CRITICAL_FROM_ISR(mask);
	} else {
		rtw_up_sema(&at_printf_sema);
		// taskEXIT_CRITICAL();
	}
	return 0;
}

int at_prt_lock_init(void) {
	rtw_init_sema(&at_printf_sema, 1);
	return 0;
}

/*
 * translate strings such as
 * "\xAA\xBB\xCC\xDD\xEE\xFF" to "AA:BB:CC:DD:EE:FF" if delim == ':'
 * "\x01\xAB\xCD\xEF\x23\x45" to "01ABCDEF2345"      if delim == 0
 */
int at_bin2hex(u8* target, int tsz, const u8* src, int ssz, u8 delim) {
	int si, ti;

	if (!src || ssz == 0 || !target || tsz == 0) {
		return -1;
	}
	for (si = 0, ti = 0; si < ssz && ti < tsz - 1; si++) {
		ti += sprintf((char*)target + ti, "%02X", src[si]);
		if (delim && ti < tsz && si != ssz - 1) {
			target[ti++] = delim;
		}
	}
	if (si < ssz) {
		return -2;
	}
	return 0;
}

/*
 * translate strings such as
 * "AA:BB:CC:DD:EE:FF" to "\xAA\xBB\xCC\xDD\xEE\xFF"
 * "01:bb:cc:dd:ee:ff" to "\x01\xBB\xCC\xDD\xEE\xFF"
 * "  \" 01AbCDEF2345" to "\x01\xAB\xCD\xEF\x23\x45"
 */
int at_hex2bin(u8* target, int tsz, const u8* src, int ssz) {
	int si, ti;

	if (!src || ssz == 0 || !target || tsz == 0) {
		return -1;
	}

	si = 0;
	while (src[si] && !isxdigit(src[si])) si++;
	for (ti = 0; si < ssz - 1 && ti < tsz;) {
		int v;
		sscanf((char *)&src[si], "%2x", &v);
		si += 2;
		target[ti++] = v;
		if (src[si] && src[si] == ':') si++;
	}

	if (ti < tsz) {
		return -2;
	}
	return 0;
}

int at_set_ipstatus(int esp_ipstat) {
	return esp_ipstatus = esp_ipstat;
}


static void init_wifi_struct(void)
{
	memset(wifi.ssid.val, 0, sizeof(wifi.ssid.val));
	memset(wifi.bssid.octet, 0, ETH_ALEN);
	memset(password, 0, sizeof(password));
	wifi.ssid.len = 0;
	wifi.password = NULL;
	wifi.password_len = 0;
	wifi.key_id = -1;
	memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
	ap.ssid.len = 0;
	ap.password = NULL;
	ap.password_len = 0;
	ap.channel = 1;
#ifdef CONFIG_FPGA
	security = -1;
#endif
}

static void print_scan_result(rtw_scan_result_t * record)
{
	#if _EN_EXPORT_ATCMD
	at_printf("%s,%d,%s,%d," MAC_FMT "", record->SSID.val, record->channel,
		  (record->security == RTW_SECURITY_OPEN) ? "Open" :
		  (record->security == RTW_SECURITY_WEP_PSK) ? "WEP" :
		  (record->security == RTW_SECURITY_WPA_TKIP_PSK) ? "WPA TKIP" :
		  (record->security == RTW_SECURITY_WPA_AES_PSK) ? "WPA AES" :
		  (record->security == RTW_SECURITY_WPA2_AES_PSK) ? "WPA2 AES" :
		  (record->security == RTW_SECURITY_WPA2_TKIP_PSK) ? "WPA2 TKIP" :
		  (record->security == RTW_SECURITY_WPA2_MIXED_PSK) ? "WPA2 Mixed" :
		  (record->security == RTW_SECURITY_WPA_WPA2_MIXED) ? "WPA/WPA2 AES" : "Unknown",
		  record->signal_strength, MAC_ARG(record->BSSID.octet));
	#else
	RTW_API_INFO("%s\t ", (record->bss_type == RTW_BSS_TYPE_ADHOC) ? "Adhoc" : "Infra");
	RTW_API_INFO(MAC_FMT, MAC_ARG(record->BSSID.octet));
	RTW_API_INFO(" %d\t ", record->signal_strength);
	RTW_API_INFO(" %d\t  ", record->channel);
	RTW_API_INFO(" %d\t  ", record->wps_type);
	RTW_API_INFO("%s\t\t ", (record->security == RTW_SECURITY_OPEN) ? "Open" :
		     (record->security == RTW_SECURITY_WEP_PSK) ? "WEP" :
		     (record->security == RTW_SECURITY_WPA_TKIP_PSK) ? "WPA TKIP" :
		     (record->security == RTW_SECURITY_WPA_AES_PSK) ? "WPA AES" :
		     (record->security == RTW_SECURITY_WPA2_AES_PSK) ? "WPA2 AES" :
		     (record->security == RTW_SECURITY_WPA2_TKIP_PSK) ? "WPA2 TKIP" :
		     (record->security == RTW_SECURITY_WPA2_MIXED_PSK) ? "WPA2 Mixed" :
		     (record->security == RTW_SECURITY_WPA_WPA2_MIXED) ? "WPA/WPA2 AES" :
	#ifdef CONFIG_SAE_SUPPORT
		     (record->security == RTW_SECURITY_WPA3_AES_PSK) ? "WP3-SAE AES" :
	#endif
		     "Unknown");

	RTW_API_INFO(" %s ", record->SSID.val);
	RTW_API_INFO("\r\n");
	#endif
}

static rtw_result_t app_scan_result_handler(rtw_scan_handler_result_t * malloced_scan_result)
{
	static int ApNum = 0;

	if (malloced_scan_result->scan_complete != RTW_TRUE) {
		rtw_scan_result_t *record = &malloced_scan_result->ap_details;
		record->SSID.val[record->SSID.len] = 0;	/* Ensure the SSID is null terminated */

		#if _EN_EXPORT_ATCMD
		at_printf("\r\nAP : %d,", ++ApNum);
		#else
		RTW_API_INFO("%d\t ", ++ApNum);
		#endif
		print_scan_result(record);

		#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
		if (malloced_scan_result->user_data)
			memcpy((void *) ((char *) malloced_scan_result->user_data +
					 (ApNum - 1) * sizeof(rtw_scan_result_t)), (char *) record,
			       sizeof(rtw_scan_result_t));
		#endif
	} else {
		#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
		inic_c2h_msg("ATWS", RTW_SUCCESS, (char *) malloced_scan_result->user_data,
			     ApNum * sizeof(rtw_scan_result_t));
		if (malloced_scan_result->user_data)
			free(malloced_scan_result->user_data);
		inic_c2h_msg("ATWS", RTW_SUCCESS, NULL, 0);
		#endif

		#if _EN_EXPORT_ATCMD
		at_printf("\r\n[ATWS] OK");
		at_printf(STR_END_OF_ATCMD_RET);
		#endif
		ApNum = 0;
	}
	return RTW_SUCCESS;
}

void fATWD(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	int timeout = 20;
	char essid[33];
	volatile int ret = RTW_SUCCESS;
	int error_no = 0;

	printf("[ATWD]: _AT_WLAN_DISC_NET_\n\r");
	printf("\n\rDeassociating AP ...");

	if (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
		printf("\n\rnot connected yet");
		goto exit_success;
	}

	#if ATCMD_VER == ATVER_2
	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl);
	#endif

	if ((ret = wifi_disconnect()) < 0) {
		printf("\n\rERROR: Operation failed!");
		error_no = 3;
		goto exit;
	}

	while (1) {
		if (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
			printf("\n\rWIFI disconnect succeed");
			break;
		}

		if (timeout == 0) {
			printf("\n\rERROR: Deassoc timeout!");
			ret = RTW_TIMEOUT;
			error_no = 4;
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout--;
	}
	printf("\n\r");

	#if CONFIG_LWIP_LAYER
	LwIP_ReleaseIP(WLAN0_IDX);
	#endif
exit:

	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	if (ret != RTW_SUCCESS)
		inic_c2h_msg("ATWD", ret, NULL, 0);
	#endif
	init_wifi_struct();
	#if ATCMD_VER == ATVER_2
	if (error_no == 0)
		at_printf("\r\n[ATWD] OK");
	else
		at_printf("\r\n[ATWD] ERROR:%d", error_no);
	#endif
	return;

exit_success:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATWD", RTW_SUCCESS, NULL, 0);
	#endif
	init_wifi_struct();

	#if ATCMD_VER == ATVER_2
	at_printf("\r\n[ATWD] OK");
	#endif
	return;
}

#if (CONFIG_INCLUDE_SIMPLE_CONFIG)
void fATWQ(void *arg)
{
	int argc = 0;
	char *argv[2] = { 0 };

	printf("[ATWQ]: _AT_WLAN_SIMPLE_CONFIG_\n\r");
	argv[argc++] = "wifi_simple_config";
	if (arg) {
		argv[argc++] = arg;
	}
	#if ATCMD_VER == ATVER_2
	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl);
	#endif
	cmd_simple_config(argc, argv);
}
#endif

#if defined(CONFIG_INCLUDE_DPP_CONFIG) && CONFIG_INCLUDE_DPP_CONFIG
void fATWq(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	char buf[256] = { 0 };
	printf("[ATWq]:\n\r");
	if (arg) {
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
	}
	cmd_dpp(argc, argv);
}
#endif

#if defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG
extern void bt_example_init(void);
void fATWb(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	printf("[ATWb]:_AT_BT_CONFIG_\n\r");
	bt_example_init();
}
#endif

void fATWS(void *arg)
{
	char buf[32] = { 0 };
	u8 *channel_list = NULL;
	u8 *pscan_config = NULL;
	int num_channel = 0;
	int i, argc = 0;
	char *argv[MAX_ARGC] = { 0 };
	volatile int ret = RTW_SUCCESS;
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	u8 *inic_scan_buf = NULL;
#endif
	int error_no = 0;

	printf("[ATWS]: _AT_WLAN_SCAN_\n\r");
	if (arg) {
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
		if (argc < 2) {
			ret = RTW_BADARG;
			error_no = 1;
			goto exit;
		}
		num_channel = atoi(argv[1]);
		channel_list = (u8 *) malloc(num_channel);
		if (!channel_list) {
			printf("[ATWS]ERROR: Can't malloc memory for channel list\n\r");
			ret = RTW_BUFFER_UNAVAILABLE_TEMPORARY;
			error_no = 2;
			goto exit;
		}
		pscan_config = (u8 *) malloc(num_channel);
		if (!pscan_config) {
			printf("[ATWS]ERROR: Can't malloc memory for pscan_config\n\r");
			ret = RTW_BUFFER_UNAVAILABLE_TEMPORARY;
			error_no = 3;
			goto exit;
		}
		//parse command channel list
		for (i = 2; i <= argc - 1; i++) {
			*(channel_list + i - 2) = (u8) atoi(argv[i]);
			*(pscan_config + i - 2) = PSCAN_ENABLE;
		}

		if ((ret = wifi_set_pscan_chan(channel_list, pscan_config, num_channel)) < 0) {
			printf("[ATWS]ERROR: wifi set partial scan channel fail\n\r");
			error_no = 4;
			goto exit;
		}
	}

	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_scan_buf = malloc(65 * sizeof(rtw_scan_result_t));
	if (inic_scan_buf == NULL) {
		ret = RTW_BUFFER_UNAVAILABLE_TEMPORARY;
		goto exit;
	}
	memset(inic_scan_buf, 0, 65 * sizeof(rtw_scan_result_t));
	if ((ret = wifi_scan_networks(app_scan_result_handler, inic_scan_buf)) != RTW_SUCCESS) {
		printf("[ATWS]ERROR: wifi scan failed\n\r");
		goto exit;
	}
	#else
	if ((ret = wifi_scan_networks(app_scan_result_handler, NULL)) != RTW_SUCCESS) {
		printf("[ATWS]ERROR: wifi scan failed\n\r");
		error_no = 5;
		goto exit;
	}
	#endif

      exit:

	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	if (ret != RTW_SUCCESS) {
		if (inic_scan_buf)
			free(inic_scan_buf);
		inic_c2h_msg("ATWS", ret, NULL, 0);
	}
	#endif

	#if ATCMD_VER == ATVER_2
	if (error_no)
		at_printf("\r\n[ATWS] ERROR:%d", error_no);
	#else
	(void) error_no;
	#endif
	if (arg && channel_list)
		free(channel_list);
	if (arg && pscan_config)
		free(pscan_config);
}

#ifdef WIFI_PERFORMANCE_MONITOR
/**
  * @brief print the time of WIFI TRx path.
  * @param  arg: the command "ATWm"
  * @retval None
  */
void fATWm(void *arg)
{
	wifi_performance_print();
}
#endif

void fATWx(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	int i = 0;

	#if CONFIG_LWIP_LAYER
	u8 *mac = LwIP_GetMAC(&xnetif[0]);
	u8 *ip = LwIP_GetIP(&xnetif[0]);
	u8 *gw = LwIP_GetGW(&xnetif[0]);
	u8 *msk = LwIP_GetMASK(&xnetif[0]);
	#endif
	u8 *ifname[2] = { (u8 *) WLAN0_NAME, (u8 *) WLAN1_NAME };
	rtw_wifi_setting_t setting;

	printf("[ATW?]: _AT_WLAN_INFO_\n\r");

	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	int ret = RTW_SUCCESS;
	int info_sz = 0;
	u8 *info = malloc(NET_IF_NUM * sizeof(rtw_wifi_setting_t) + 3 * sizeof(rtw_mac_t));
	if (info == NULL)
		ret = RTW_BUFFER_UNAVAILABLE_TEMPORARY;
	#endif
	for (i = 0; i < NET_IF_NUM; i++) {
		if (rltk_wlan_running(i)) {

			#if CONFIG_LWIP_LAYER
			mac = LwIP_GetMAC(&xnetif[i]);
			ip = LwIP_GetIP(&xnetif[i]);
			gw = LwIP_GetGW(&xnetif[i]);
			msk = LwIP_GetMASK(&xnetif[i]);
			#endif
			printf("\n\r\nWIFI %s Status: Running", ifname[i]);
			printf("\n\r==============================");

			rltk_wlan_statistic(i);

			wifi_get_setting((const char *) ifname[i], &setting);
			wifi_show_setting((const char *) ifname[i], &setting);

			#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
			if (info) {
				memcpy(info + info_sz, (void *) &setting, sizeof(rtw_wifi_setting_t));
				info_sz += sizeof(rtw_wifi_setting_t);
			}
			#endif

			#if CONFIG_LWIP_LAYER
			#if _EN_EXPORT_ATCMD
			at_printf(MAC_FMT ",", MAC_ARG(mac));
			at_printf("%d.%d.%d.%d,", ip[0], ip[1], ip[2], ip[3]);
			at_printf("%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);
			#endif
			printf("\n\rInterface (%s)", ifname[i]);
			printf("\n\r==============================");
			printf("\n\r\tMAC => " MAC_FMT, MAC_ARG(mac));
			printf("\n\r\tIP  => %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			printf("\n\r\tGW  => %d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);
			printf("\n\r\tmsk  => %d.%d.%d.%d\n\r", msk[0], msk[1], msk[2], msk[3]);
			#endif

			if (setting.mode == RTW_MODE_AP || i == 1) {
				int client_number;
				struct {
					int count;
					rtw_mac_t mac_list[AP_STA_NUM];
				} client_info;

				client_info.count = AP_STA_NUM;
				wifi_get_associated_client_list(&client_info, sizeof(client_info));

				printf("\n\rAssociated Client List:");
				printf("\n\r==============================");

				if (client_info.count == 0)
					printf("\n\rClient Num: %d\n\r", client_info.count);
				else {
					printf("\n\rClient Num: %d", client_info.count);
					for (client_number = 0; client_number < client_info.count; client_number++) {
						printf("\n\rClient %d:", client_number + 1);
						printf("\n\r\tMAC => " MAC_FMT "",
						       MAC_ARG(client_info.mac_list[client_number].octet));

						#if _EN_EXPORT_ATCMD
						at_printf("\r\nCLIENT : %d," MAC_FMT "", client_number + 1,
							  MAC_ARG(client_info.mac_list[client_number].octet));
						#endif

						#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
						if (info) {
							memcpy(info + info_sz,
							       (void *) &client_info.mac_list[client_number],
							       sizeof(rtw_mac_t));
							info_sz += sizeof(rtw_mac_t);
						}
						#endif
					}
					printf("\n\r");
				}
			}
		}

		// show the ethernet interface info
		else {
			#if CONFIG_ETHERNET
			if (i == NET_IF_NUM - 1) {
				#if CONFIG_LWIP_LAYER
				mac = LwIP_GetMAC(&xnetif[i]);
				ip = LwIP_GetIP(&xnetif[i]);
				gw = LwIP_GetGW(&xnetif[i]);
				printf("\n\rInterface ethernet\n");
				printf("\n\r==============================");
				printf("\n\r\tMAC => " MAC_FMT, MAC_ARG(mac));
				printf("\n\r\tIP  => %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
				printf("\n\r\tGW  => %d.%d.%d.%d\n\r", gw[0], gw[1], gw[2], gw[3]);
				#endif	// end CONFIG_LWIP_LAYER
			}
			#endif	// end CONFIG_ETHERNET
		}
	}

	#if defined(configUSE_TRACE_FACILITY) && (configUSE_TRACE_FACILITY == 1)
	trace_task();
	#endif

	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	if (ret != RTW_SUCCESS)
		inic_c2h_msg("ATW?", ret, NULL, 0);
	else
		inic_c2h_msg("ATW?", RTW_SUCCESS, (char *) info, info_sz);

	if (info)
		free(info);
	info = NULL;
	#endif

	#if _EN_EXPORT_ATCMD
	at_printf("\r\n[ATW?] OK");
	#endif

}

#if ATCMD_VER == ATVER_1
void fATW0(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW0]Usage: ATW0=SSID\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	printf("[ATW0]: _AT_WLAN_SET_SSID_ [%s]\n\r", (char *) arg);
	strcpy((char *) wifi.ssid.val, (char *) arg);
	wifi.ssid.len = strlen((char *) arg);
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW0", ret, NULL, 0);
	#endif
	return;
}

void fATW1(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW1]Usage: ATW1=PASSPHRASE\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	printf("[ATW1]: _AT_WLAN_SET_PASSPHRASE_ [%s]\n\r", (char *) arg);

	#ifdef CONFIG_SAE_SUPPORT
	if (strlen((char *) arg) > 63) {
		printf("[ATW1]: Error: password input(%d) > 63 \n\r", strlen((char *) arg));
		goto exit;
	}
	#endif

	strcpy((char *) password, (char *) arg);
	wifi.password = password;
	wifi.password_len = strlen((char *) arg);
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW1", ret, NULL, 0);
	#endif
	return;
}

void fATW2(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW2]Usage: ATW2=KEYID\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	printf("[ATW2]: _AT_WLAN_SET_KEY_ID_ [%s]\n\r", (char *) arg);
	if ((strlen((const char *) arg) != 1) || (*(char *) arg < '0' || *(char *) arg > '3')) {
		printf("\n\rWrong WEP key id. Must be one of 0,1,2, or 3.");
		ret = RTW_BADARG;
		goto exit;
	}
	wifi.key_id = atoi((const char *) (arg));
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW2", ret, NULL, 0);
	#endif
	return;
}

void fATW3(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW3]Usage: ATW3=SSID\n\r");
		ret = RTW_BADARG;
		goto exit;
	}

	ap.ssid.len = strlen((char *) arg);

	if (ap.ssid.len > 32) {
		printf("[ATW3]Error: SSID length can't exceed 32\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	strcpy((char *) ap.ssid.val, (char *) arg);

	printf("[ATW3]: _AT_WLAN_AP_SET_SSID_ [%s]\n\r", ap.ssid.val);
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW3", ret, NULL, 0);
	#endif
	return;
}

void fATW4(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW4]Usage: ATW4=PASSWORD\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	strcpy((char *) password, (char *) arg);
	ap.password = password;
	ap.password_len = strlen((char *) arg);
	printf("[ATW4]: _AT_WLAN_AP_SET_SEC_KEY_ [%s]\n\r", ap.password);
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW4", ret, NULL, 0);
	#endif
	return;
}

void fATW5(void *arg)
{
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW5]Usage: ATW5=CHANNEL\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	ap.channel = (unsigned char) atoi((const char *) arg);
	printf("[ATW5]: _AT_WLAN_AP_SET_CHANNEL_ [channel %d]\n\r", ap.channel);
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW5", ret, NULL, 0);
	#endif
	return;
}

void fATW6(void *arg)
{
	u32 mac[ETH_ALEN];
	u32 i;
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	if (!arg) {
		printf("[ATW6]Usage: ATW6=BSSID\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	printf("[ATW6]: _AT_WLAN_SET_BSSID_ [%s]\n\r", (char *) arg);
	at_hex2bin(wifi.bssid.octet, ETH_ALEN, arg, strlen(arg));

exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW6", ret, NULL, 0);
	#endif
	return;
}

#ifdef CONFIG_FPGA
void fATW7(void *arg)
{
	if (!arg) {
		printf("[ATW7]Usage: ATW7=0,1,2 or 3(open, WEP, TKIP or AES)\n\r");
		return;
	}
	volatile int ret = RTW_SUCCESS;
	(void) ret;
	printf("[ATW7]: _AT_WLAN_SET_SECURITY [%s]\n\r", (char *) arg);
	if ((strlen((const char *) arg) != 1) || (*(char *) arg < '0' || *(char *) arg > '3')) {
		printf("\n\rWrong num. Must be one of 0,1,2 or 3.");
		ret = RTW_BADARG;
		goto exit;
	}
	security = atoi((const char *) (arg));
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATW7", ret, NULL, 0);
	#endif
	return;
}
#endif

void fATWA(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;
	#if CONFIG_LWIP_LAYER
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif *pnetif = &xnetif[0];
	#endif
	int timeout = 20;
	volatile int ret = RTW_SUCCESS;
	printf("[ATWA]: _AT_WLAN_AP_ACTIVATE_\n\r");
	if (ap.ssid.val[0] == 0) {
		printf("[ATWA]Error: SSID can't be empty\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	if (ap.password == NULL) {
		ap.security_type = RTW_SECURITY_OPEN;
	} else {
		if (ap.password_len <= RTW_MAX_PSK_LEN && ap.password_len >= RTW_MIN_PSK_LEN) {
			ap.security_type = RTW_SECURITY_WPA2_AES_PSK;
			if (ap.password_len == RTW_MAX_PSK_LEN) {	//password_len=64 means pre-shared key, pre-shared key should be 64 hex characters
				unsigned char i, j;
				for (i = 0; i < 64; i++) {
					j = ap.password[i];
					if (!((j >= '0' && j <= '9') || (j >= 'A' && j <= 'F') || (j >= 'a' && j <= 'f'))) {
						printf("[ATWA]Error: password should be 64 hex characters or 8-63 ASCII characters\n\r");
						ret = RTW_INVALID_KEY;
						goto exit;
					}
				}
			}
		}
		#ifdef CONFIG_FPGA
		else if (ap.password_len == 5) {
			ap.security_type = RTW_SECURITY_WEP_PSK;
		}
		#endif
		else {
			printf("[ATWA]Error: password should be 64 hex characters or 8-63 ASCII characters\n\r");
			ret = RTW_INVALID_KEY;
			goto exit;
		}
	}
#ifdef CONFIG_FPGA
	if (security == 0)
		ap.security_type = RTW_SECURITY_OPEN;
	else if (security == 1)
		ap.security_type = RTW_SECURITY_WEP_PSK;
	else if (security == 2)
		ap.security_type = RTW_SECURITY_WPA2_TKIP_PSK;
	else if (security == 3)
		ap.security_type = RTW_SECURITY_WPA2_AES_PSK;
#endif

#if CONFIG_LWIP_LAYER
	dhcps_deinit();
#if LWIP_VERSION_MAJOR >= 2
	IP4_ADDR(ip_2_ip4(&ipaddr), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	IP4_ADDR(ip_2_ip4(&netmask), NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
	IP4_ADDR(ip_2_ip4(&gw), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	netif_set_addr(pnetif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
#else
	IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
	IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	netif_set_addr(pnetif, &ipaddr, &netmask, &gw);
#endif
#ifdef CONFIG_DONT_CARE_TP
	pnetif->flags |= NETIF_FLAG_IPSWITCH;
#endif
#endif
	wifi_off();
	vTaskDelay(20);
	if (wifi_on(RTW_MODE_AP) < 0) {
		printf("\n\rERROR: Wifi on failed!");
		ret = RTW_ERROR;
		goto exit;
	}
	printf("\n\rStarting AP ...");

#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
	wpas_wps_dev_config(pnetif->hwaddr, 1);
#endif
	if ((ret =
	     wifi_start_ap((char *) ap.ssid.val, ap.security_type, (char *) ap.password, ap.ssid.len, ap.password_len,
			   ap.channel)) < 0) {
		printf("\n\rERROR: Operation failed!");
		goto exit;
	}

	while (1) {
		char essid[33];

		if (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) > 0) {
			if (strcmp((const char *) essid, (const char *) ap.ssid.val) == 0) {
				printf("\n\r%s started\n", ap.ssid.val);
				ret = RTW_SUCCESS;
				break;
			}
		}

		if (timeout == 0) {
			printf("\n\rERROR: Start AP timeout!");
			ret = RTW_TIMEOUT;
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout--;
	}

#if defined( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE )&&( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE == 1 )
	wifi_set_ap_polling_sta(1);
#endif

#if CONFIG_LWIP_LAYER
	//LwIP_UseStaticIP(pnetif);
	dhcps_init(pnetif);
#endif

      exit:
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_wifi_info("ATWA", ret);
#endif
	init_wifi_struct();
}

#if CONFIG_INIC_EN
static int _find_ap_from_scan_buf(char *buf, int buflen, char *target_ssid, void *user_data)
{
	rtw_wifi_setting_t *pwifi = (rtw_wifi_setting_t *) user_data;
	int plen = 0;

	while (plen < buflen) {
		u8 len, ssid_len, security_mode;
		char *ssid;

		// len offset = 0
		len = (int) *(buf + plen);
		// check end
		if (len == 0)
			break;
		// ssid offset = 14
		ssid_len = len - 14;
		ssid = buf + plen + 14;
		if ((ssid_len == strlen(target_ssid))
		    && (!memcmp(ssid, target_ssid, ssid_len))) {
			strcpy((char *) pwifi->ssid, target_ssid);
			// channel offset = 13
			pwifi->channel = *(buf + plen + 13);
			// security_mode offset = 11
			security_mode = (u8) * (buf + plen + 11);
			if (security_mode == IW_ENCODE_ALG_NONE)
				pwifi->security_type = RTW_SECURITY_OPEN;
			else if (security_mode == IW_ENCODE_ALG_WEP)
				pwifi->security_type = RTW_SECURITY_WEP_PSK;
			else if (security_mode == IW_ENCODE_ALG_CCMP)
				pwifi->security_type = RTW_SECURITY_WPA2_AES_PSK;
			break;
		}
		plen += len;
	}
	return 0;
}

int _get_ap_security_mode(IN char *ssid, OUT rtw_security_t * security_mode, OUT u8 * channel)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1000;

	memset(&wifi, 0, sizeof(wifi));

	if (wifi_scan_networks_with_ssid(_find_ap_from_scan_buf, (void *) &wifi, scan_buflen, ssid, strlen(ssid)) !=
	    RTW_SUCCESS) {
		printf("Wifi scan failed!\n");
		return 0;
	}

	if (strcmp(wifi.ssid, ssid) == 0) {
		*security_mode = wifi.security_type;
		*channel = wifi.channel;
		return 1;
	}

	return 0;
}
#endif

void fATWC(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	int mode, ret;
	unsigned long tick1 = xTaskGetTickCount();
	unsigned long tick2, tick3;
	char empty_bssid[6] = { 0 }, assoc_by_bssid = 0;

	printf("[ATWC]: _AT_WLAN_JOIN_NET_\n\r");
	if (memcmp(wifi.bssid.octet, empty_bssid, 6))
		assoc_by_bssid = 1;
	else if (wifi.ssid.val[0] == 0) {
		printf("[ATWC]Error: SSID can't be empty\n\r");
		ret = RTW_BADARG;
		goto EXIT;
	}
	if (wifi.password != NULL) {
		if ((wifi.key_id >= 0) && (wifi.key_id <= 3)) {
			wifi.security_type = RTW_SECURITY_WEP_PSK;
		} else {
			wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
		}
	} else {
		wifi.security_type = RTW_SECURITY_OPEN;
	}
	//Check if in AP mode
	wext_get_mode(WLAN0_NAME, &mode);
	if (mode == IW_MODE_MASTER) {
#if CONFIG_LWIP_LAYER
		dhcps_deinit();
#endif
		wifi_off();
		vTaskDelay(20);
		if (wifi_on(RTW_MODE_STA) < 0) {
			printf("\n\rERROR: Wifi on failed!");
			ret = RTW_ERROR;
			goto EXIT;
		}
	}
#if CONFIG_INIC_EN		//get security mode from scan list
	u8 connect_channel;
	u8 pscan_config;
	//the keyID may be not set for WEP which may be confued with WPA2
	if ((wifi.security_type == RTW_SECURITY_UNKNOWN) || (wifi.security_type == RTW_SECURITY_WPA2_AES_PSK)) {
		int security_retry_count = 0;
		while (1) {
			if (_get_ap_security_mode((char *) wifi.ssid.val, &wifi.security_type, &connect_channel))
				break;
			security_retry_count++;
			if (security_retry_count >= 3) {
				printf("Can't get AP security mode and channel.\n");
				ret = RTW_NOTFOUND;
				goto EXIT;
			}
		}
		if (wifi.security_type == RTW_SECURITY_WEP_PSK || wifi.security_type == RTW_SECURITY_WEP_SHARED)
			wifi.key_id = (wifi.key_id < 0 || wifi.key_id > 3) ? 0 : wifi.key_id;
#if 0				//implemented in wifi_connect()
		//hex to ascii conversion
		if (wifi.security_type == RTW_SECURITY_WEP_PSK) {
			u8 pwd[14] = {0};
			if (at_hex2bin(pwd, 13, wifi.password, strlen(wifi.password)) >= 0) {
				strcpy((char *) wifi.password, (char *) pwd);
				wifi.password_len = 13;
			} else
			if (at_hex2bin(pwd, 5, wifi.password, strlen(wifi.password)) >= 0) {
				strcpy((char *) wifi.password, (char *) pwd);
				wifi.password_len = 5;
			}
		}
#endif
	}
	pscan_config = PSCAN_ENABLE;
	if (connect_channel > 0 && connect_channel < 14)
		wifi_set_pscan_chan(&connect_channel, &pscan_config, 1);
#endif

	if (assoc_by_bssid) {
		printf("\n\rJoining BSS by BSSID " MAC_FMT " ...\n\r", MAC_ARG(wifi.bssid.octet));
		ret =
		    wifi_connect_bssid(wifi.bssid.octet, (char *) wifi.ssid.val, wifi.security_type,
				       (char *) wifi.password, ETH_ALEN, wifi.ssid.len, wifi.password_len, wifi.key_id,
				       NULL);
	} else {
		printf("\n\rJoining BSS by SSID %s...\n\r", (char *) wifi.ssid.val);
		ret = wifi_connect((char *) wifi.ssid.val, wifi.security_type, (char *) wifi.password, wifi.ssid.len,
				   wifi.password_len, wifi.key_id, NULL);
	}

	if (ret != RTW_SUCCESS) {
		if (ret == RTW_INVALID_KEY)
			printf("\n\rERROR:Invalid Key ");

		printf("\n\rERROR: Can't connect to AP");
		goto EXIT;
	}
	tick2 = xTaskGetTickCount();
	printf("\r\nConnected after %dms.\n", (tick2 - tick1));
#if CONFIG_LWIP_LAYER
	/* Start DHCPClient */
	LwIP_DHCP(0, DHCP_START);
	tick3 = xTaskGetTickCount();
	printf("\r\n\nGot IP after %dms.\n", (tick3 - tick1));
#endif
	printf("\n\r");
      EXIT:
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_wifi_info("ATWC", ret);
#endif

	init_wifi_struct();
}

#if SCAN_WITH_SSID
void fATWs(void *arg)
{
	char buf[32] = { 0 };
	u8 *channel_list = NULL;
	u8 *pscan_config = NULL;
	int scan_buf_len = 500;
	int num_channel = 0;
	int i, argc = 0;
	char *argv[MAX_ARGC] = { 0 };
	printf("[ATWs]: _AT_WLAN_SCAN_WITH_SSID_ [%s]\n\r", (char *) wifi.ssid.val);
	if (arg) {
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
		if (argc == 2) {
			scan_buf_len = atoi(argv[1]);
			if (scan_buf_len < 36) {
				printf("[ATWs] BUFFER_LENGTH too short\n\r");
				goto exit;
			}
		} else if (argc > 2) {
			num_channel = atoi(argv[1]);
			channel_list = (u8 *) malloc(num_channel);
			if (!channel_list) {
				printf("[ATWs]ERROR: Can't malloc memory for channel list\n\r");
				goto exit;
			}
			pscan_config = (u8 *) malloc(num_channel);
			if (!pscan_config) {
				printf("[ATWs]ERROR: Can't malloc memory for pscan_config\n\r");
				goto exit;
			}
			//parse command channel list
			for (i = 2; i <= argc - 1; i++) {
				*(channel_list + i - 2) = (u8) atoi(argv[i]);
				*(pscan_config + i - 2) = PSCAN_ENABLE;
			}

			if (wifi_set_pscan_chan(channel_list, pscan_config, num_channel) < 0) {
				printf("[ATWs]ERROR: wifi set partial scan channel fail\n\r");
				goto exit;
			}
		}
	} else {
		printf("[ATWs]For Scan all channel Usage: ATWs=BUFFER_LENGTH\n\r");
		printf("[ATWs]For Scan partial channel Usage: ATWs=num_channels[channel_num1, ...]\n\r");
		goto exit;
	}

	if (wifi_scan_networks_with_ssid(NULL, NULL, scan_buf_len, (char *) wifi.ssid.val, wifi.ssid.len) !=
	    RTW_SUCCESS) {
		printf("[ATWs]ERROR: wifi scan failed\n\r");
	}
      exit:
	init_wifi_struct();
	if (arg && channel_list)
		free(channel_list);
	if (arg && pscan_config)
		free(pscan_config);
}
#endif

void fATWR(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	int rssi = 0;
	printf("[ATWR]: _AT_WLAN_GET_RSSI_\n\r");
	wifi_get_rssi(&rssi);
	printf("\n\rwifi_get_rssi: rssi = %d", rssi);
	printf("\n\r");
}

void fATWP(void *arg)
{
	if (!arg) {
		printf("[ATWP]Usage: ATWP=0/1\n\r");
		return;
	}
	unsigned int parm = atoi((const char *) (arg));
	printf("[ATWP]: _AT_WLAN_POWER_[%s]\n\r", parm ? "ON" : "OFF");
	if (parm == 1) {
		if (wifi_on(RTW_MODE_STA) < 0) {
			printf("\n\rERROR: Wifi on failed!\n");
		}
	} else if (parm == 0) {
		wifi_off();
	} else
		printf("[ATWP]Usage: ATWP=0/1\n\r");
}

#if CONFIG_WOWLAN_SERVICE
//for wowlan setting
void fATWV(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	printf("[ATWV]: _AT_WLAN_WOWLAN_\r\n");

	argc = parse_param(arg, argv);

	cmd_wowlan_service(argc, argv);

	return;
}
#endif

#ifdef  CONFIG_CONCURRENT_MODE
void fATWB(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	int timeout = 20;	//, mode;
	volatile int ret = RTW_SUCCESS;
#if CONFIG_LWIP_LAYER
	struct netif *pnetiff = (struct netif *) &xnetif[1];
#endif
	printf("[ATWB](_AT_WLAN_AP_STA_ACTIVATE_)\n\r");
	if (ap.ssid.val[0] == 0) {
		printf("[ATWB]Error: SSID can't be empty\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	if (ap.password == NULL) {
		ap.security_type = RTW_SECURITY_OPEN;
	} else {
		if (ap.password_len <= RTW_MAX_PSK_LEN && ap.password_len >= RTW_MIN_PSK_LEN) {
			ap.security_type = RTW_SECURITY_WPA2_AES_PSK;
			if (ap.password_len == RTW_MAX_PSK_LEN) {	//password_len=64 means pre-shared key, pre-shared key should be 64 hex characters
				unsigned char i, j;
				for (i = 0; i < 64; i++) {
					j = ap.password[i];
					if (!
					    ((j >= '0' && j <= '9') || (j >= 'A' && j <= 'F')
					     || (j >= 'a' && j <= 'f'))) {
						printf
						    ("[ATWA]Error: password should be 64 hex characters or 8-63 ASCII characters\n\r");
						ret = RTW_INVALID_KEY;
						goto exit;
					}
				}
			}
		} else {
			printf("[ATWA]Error: password should be 64 hex characters or 8-63 ASCII characters\n\r");
			ret = RTW_INVALID_KEY;
			goto exit;
		}
	}
#if CONFIG_LWIP_LAYER
	dhcps_deinit();
#endif

	wifi_off();
	vTaskDelay(20);
	if ((ret = wifi_on(RTW_MODE_STA_AP)) < 0) {
		printf("\n\rERROR: Wifi on failed!");
		ret = RTW_ERROR;
		goto exit;
	}

	printf("\n\rStarting AP ...");
	if ((ret =
	     wifi_start_ap((char *) ap.ssid.val, ap.security_type, (char *) ap.password, ap.ssid.len, ap.password_len,
			   ap.channel)) < 0) {
		printf("\n\rERROR: Operation failed!");
		goto exit;
	}
	while (1) {
		char essid[33];

		if (wext_get_ssid(WLAN1_NAME, (unsigned char *) essid) > 0) {
			if (strcmp((const char *) essid, (const char *) ap.ssid.val) == 0) {
				printf("\n\r%s started\n", ap.ssid.val);
				ret = RTW_SUCCESS;
				break;
			}
		}

		if (timeout == 0) {
			printf("\n\rERROR: Start AP timeout!");
			ret = RTW_TIMEOUT;
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout--;
	}
	#if CONFIG_LWIP_LAYER
	LwIP_UseStaticIP(&xnetif[1]);
	#ifdef CONFIG_DONT_CARE_TP
	pnetiff->flags |= NETIF_FLAG_IPSWITCH;
	#endif
	dhcps_init(pnetiff);
	#endif

	#if defined( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE )&&( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE == 1 )
	wifi_set_ap_polling_sta(1);
	#endif
      exit:
	#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_wifi_info("ATWB", ret);
	#endif
	init_wifi_struct();
}
#endif

#ifdef CONFIG_PROMISC
void fATWM(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	argv[0] = "wifi_promisc";
	printf("[ATWM]: _AT_WLAN_PROMISC_\n\r");
	if (!arg) {
		printf("[ATWM]Usage: ATWM=DURATION_SECONDS[with_len]");
		#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
		inic_c2h_msg("ATWM", RTW_BADARG, NULL, 0);
		#endif
		return;
	}
	if ((argc = parse_param(arg, argv)) > 1) {
		cmd_promisc(argc, argv);
	}
}
#endif

void fATWW(void *arg)
{
#if CONFIG_ENABLE_WPS
	int argc = 0;
	char *argv[4];
	printf("[ATWW]: _AT_WLAN_WPS_\n\r");
	if (!arg) {
		printf("[ATWW]Usage: ATWW=pbc/pin\n\r");
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
		inic_c2h_msg("ATWW", RTW_BADARG, NULL, 0);
#endif
		return;
	}
	argv[argc++] = "wifi_wps";
	argv[argc++] = arg;
	cmd_wps(argc, argv);
#else
	printf("Please set CONFIG_ENABLE_WPS 1 in platform_opts.h to enable ATWW command\n");
#endif
}

void fATWw(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
	int argc = 0;
	char *argv[4];
	printf("[ATWw]: _AT_WLAN_AP_WPS_\n\r");
	if (!arg) {
		printf("[ATWw]Usage: ATWw=pbc/pin\n\r");
		return;
	}
	argv[argc++] = "wifi_ap_wps";
	argv[argc++] = arg;
	cmd_ap_wps(argc, argv);
#endif
}

#if CONFIG_ENABLE_P2P
void fATWG(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWG]: _AT_WLAN_P2P_START_\n\r");
	argv[argc++] = "p2p_start";
	cmd_wifi_p2p_start(argc, argv);
}

void fATWg(void *arg)
{
	int argc = 0;
	char *argv[4];
	int ret = 0;
	printf("[ATWg]: _AT_WLAN_P2P_AUTO_GO_START_\n\r");
	argv[argc++] = "p2p_auto_go_start";
	ret = cmd_wifi_p2p_auto_go_start(argc, argv);
	if (ret < 0)
		printf("\r\n[ATWG]: Nothing to do. Please enter ATWG to initialize P2P.\n\r");
}

void fATWH(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWH]: _AT_WLAN_P2P_STOP_\n\r");
	argv[argc++] = "p2p_stop";
	cmd_wifi_p2p_stop(argc, argv);
}

void fATWJ(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWJ]: _AT_WLAN_P2P_CONNECT_\n\r");
	argv[0] = "p2p_connect";
	if (!arg) {
		printf("ATWc=[DEST_MAC,pbc/pin]\n\r");
		return;
	}
	if ((argc = parse_param(arg, argv)) > 1) {
		cmd_p2p_connect(argc, argv);
	}
}

void fATWK(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWK]: _AT_WLAN_P2P_DISCONNECT_\n\r");
	argv[argc++] = "p2p_disconnect";
	cmd_p2p_disconnect(argc, argv);
}

void fATWN(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWN]: _AT_WLAN_P2P_INFO_\n\r");
	argv[argc++] = "p2p_info";
	cmd_p2p_info(argc, argv);
}

void fATWF(void *arg)
{
	int argc = 0;
	char *argv[4];
	printf("[ATWF]: _AT_WLAN_P2P_FIND_\n\r");
	argv[argc++] = "p2p_find";
	cmd_p2p_find(argc, argv);
}
#endif
#if CONFIG_OTA_UPDATE
void fATWO(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = { 0 };
	printf("[ATWO]: _AT_WLAN_OTA_UPDATE_\n\r");
	if (!arg) {
		printf("[ATWO]Usage: ATWO=IP[PORT] or ATWO= REPOSITORY[FILE_PATH]\n\r");
		return;
	}
	argv[0] = "update";
	if ((argc = parse_param(arg, argv)) != 3) {
		printf("[ATWO]Usage: ATWO=IP[PORT] or ATWO= REPOSITORY[FILE_PATH]\n\r");
		return;
	}
	cmd_update(argc, argv);
}
#endif

#if CONFIG_AIRKISS
void fATWX(void *arg)
{
	int argc;
	int ret = RTW_SUCCESS;
	unsigned char *argv[MAX_ARGC] = { 0 };

	argv[0] = "airkiss";
	argc = parse_param(arg, argv);
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			ret = airkiss_start(NULL);
		} else if (strcmp(argv[1], "stop") == 0) {
			ret = airkiss_stop();
		} else {
			printf("\r\n[ATWX] Usage: ATWX=[start/stop]");
		}
	} else {
		printf("\r\n[ATWX] start/stop airkiss config\r\n");
		printf("\r\n[ATWX] Usage: ATWX=[start/stop]");
		ret = RTW_ERROR;
	}

#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	if (ret != RTW_SUCCESS)
		inic_c2h_msg("ATWX", RTW_ERROR, NULL, 0);
#endif
}
#endif

void fATWZ(void *arg)
{
	char buf[32] = { 0 };
	char *copy = buf;
	int i = 0;
	int len = 0;
	volatile int ret = RTW_SUCCESS;
	(void) ret;

	printf("[ATWZ]: _AT_WLAN_IWPRIV_\n\r");
	if (!arg) {
		printf("[ATWZ]Usage: ATWZ=COMMAND[PARAMETERS]\n\r");
		ret = RTW_BADARG;
		goto exit;
	}
	strcpy(copy, arg);
	len = strlen(copy);
	do {
		if ((*(copy + i) == '['))
			*(copy + i) = ' ';
		if ((*(copy + i) == ']') || (*(copy + i) == '\0')) {
			*(copy + i) = '\0';
			break;
		}
	} while ((i++) < len);

	i = 0;
	do {
		if ((*(copy + i) == ',')) {
			*(copy + i) = ' ';
			break;
		}
	} while ((i++) < len);

#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	ret = wext_private_command_with_retval(WLAN0_NAME, copy, buf, 32);
	printf("\n\rPrivate Message: %s", (char *) buf);
	if (ret == RTW_SUCCESS)
		inic_c2h_msg("ATWZ", ret, buf, strlen(buf));
#else
	wext_private_command(WLAN0_NAME, copy, 1);
#endif
      exit:
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	if (ret != RTW_SUCCESS)
		inic_c2h_msg("ATWZ", ret, NULL, 0);
#endif
	return;			// exit label cannot be last statement
}

#ifdef CONFIG_POWER_SAVING
void fATXP(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = { 0 };
	volatile int ret = 0;
	(void) ret;
	int mode, dtim;
	int tdma_slot_period, tdma_rfon_period_len_1, tdma_rfon_period_len_2, tdma_rfon_period_len_3;
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	char *res = NULL;
	int res_len = 0;
#endif

	printf("[ATXP]: _AT_WLAN_POWER_MODE_\r\n");

	if (!arg) {
		printf("[ATXP] Usage: ATXP=lps/ips/dtim/tdma/bcn_mode[mode]\r\n");
		ret = RTW_BADARG;
		goto exit;
	} else {
		argc = parse_param(arg, argv);
		if (argc < 3) {
			printf("[ATXP] Usage: ATXP=lps/ips/dtim/tdma/bcn_mode[mode]\r\n");
			ret = RTW_BADARG;
			goto exit;
		}
	}

	if (strcmp(argv[1], "lps") == 0) {
		mode = atoi(argv[2]);
		if (mode >= 0 && mode < 0xFF) {
			printf("lps mode:%d\r\n", mode);
			wifi_set_power_mode(0xff, mode);
		}
	}

	if (strcmp(argv[1], "ips") == 0) {
		mode = atoi(argv[2]);
		if (mode >= 0 && mode < 0xFF) {
			printf("ips mode:%d\r\n", mode);
			wifi_set_power_mode(mode, 0xFF);
		}
	}

	if (strcmp(argv[1], "tdma") == 0) {
		if (argc >= 6) {
			tdma_slot_period = atoi(argv[2]);
			tdma_rfon_period_len_1 = atoi(argv[3]);
			tdma_rfon_period_len_2 = atoi(argv[4]);
			tdma_rfon_period_len_3 = atoi(argv[5]);
			printf("tdma param: %d %d %d %d\r\n", tdma_slot_period, tdma_rfon_period_len_1,
			       tdma_rfon_period_len_2, tdma_rfon_period_len_3);
			wifi_set_tdma_param(tdma_slot_period, tdma_rfon_period_len_1, tdma_rfon_period_len_2,
					    tdma_rfon_period_len_3);
		}
	}

	if (strcmp(argv[1], "dtim") == 0) {
		dtim = atoi(argv[2]);
		printf("dtim: %d\r\n", dtim);
		wifi_set_lps_dtim(dtim);
	}

	if (strcmp(argv[1], "bcn_mode") == 0) {
		mode = atoi(argv[2]);
		printf("Beacon mode: %d\r\n", mode);
		wifi_set_beacon_mode(mode);
	}

	if (strcmp(argv[1], "lps_thresh") == 0) {
		mode = atoi(argv[2]);
		printf("LPS thresh: ");
		if (mode == 0)
			printf("packet count threshold\n\r");
		else if (mode == 1)
			printf("enter lps directly\n\r");
		else
			printf("tp threshold\n\r");
		wifi_set_lps_thresh(mode);
	}

	if (strcmp(argv[1], "lps_level") == 0) {
		mode = atoi(argv[2]);
		printf("lps_level: %d\r\n", mode);
		wifi_set_lps_level(mode);
	}

	if (strcmp(argv[1], "get") == 0) {
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
		char buf[32];
		int index = 0;
		memset(buf, 0, sizeof(buf));
		snprintf(buf, 32, "%s,%s,", argv[1], argv[2]);
		index = strlen(buf);
#endif
		if (strcmp(argv[2], "dtim") == 0) {
			wifi_get_lps_dtim((unsigned char *) &dtim);
			printf("get dtim: %d\r\n", (unsigned char) dtim);
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
			sprintf(buf + index, "0x%02x", (unsigned char) dtim);
			res = (char *) buf;
			res_len = strlen(buf);
#endif
		}
	}

      exit:
#if defined(CONFIG_INIC_CMD_RSP) && CONFIG_INIC_CMD_RSP
	inic_c2h_msg("ATXP", ret, res, res_len);
	res = NULL;
	res_len = 0;
#endif
	return;
}
#endif

void print_wlan_help(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	printf("\n\rWLAN AT COMMAND SET:");
	printf("\n\r==============================");
	printf("\n\r1. Wlan Scan for Network Access Point");
	printf("\n\r   # ATWS");
	printf("\n\r2. Connect to an AES AP");
	printf("\n\r   # ATW0=SSID");
	printf("\n\r   # ATW1=PASSPHRASE");
	printf("\n\r   # ATWC");
	printf("\n\r3. Create an AES AP");
	printf("\n\r   # ATW3=SSID");
	printf("\n\r   # ATW4=PASSPHRASE");
	printf("\n\r   # ATW5=CHANNEL");
	printf("\n\r   # ATWA");
	printf("\n\r4. Ping");
	printf("\n\r   # ATWI=xxx.xxx.xxx.xxx");
}

#if WIFI_LOGO_CERTIFICATION_CONFIG
u8 use_static_ip = 0;
void fATPE(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
	unsigned int ip_addr = 0;
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPE] Usage : ATPE=<ip>(,<gateway>,<mask>)");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);

	if ((argc > 4) || (argc < 2)) {
		//at_printf("\r\n[ATPE] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
		ip_addr = inet_addr(argv[1]);
		IP4_ADDR(ip_2_ip4(&ipaddr), ip_addr & 0xff, (ip_addr >> 8) & 0xff, (ip_addr >> 16) & 0xff,
			 (ip_addr >> 24) & 0xff);
	} else {
		//at_printf("\r\n[ATPE] ERROR : parameter format error");
		error_no = 2;
		goto exit;
	}

	if (argv[2] != NULL) {
		ip_addr = inet_addr(argv[2]);
		IP4_ADDR(ip_2_ip4(&gw), ip_addr & 0xff, (ip_addr >> 8) & 0xff, (ip_addr >> 16) & 0xff,
			 (ip_addr >> 24) & 0xff);

	}

	if (argv[3] != NULL) {
		ip_addr = inet_addr(argv[3]);
		IP4_ADDR(ip_2_ip4(&netmask), ip_addr & 0xff, (ip_addr >> 8) & 0xff, (ip_addr >> 16) & 0xff,
			 (ip_addr >> 24) & 0xff);

	}
	//IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
	netif_set_addr(&xnetif[0], ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

      exit:
	if (error_no == 0) {
		at_printf("\r\n[ATPE] OK");
	  	use_static_ip = 1;
	} else
		at_printf("\r\n[ATPE] ERROR:%d", error_no);

	return;

}

#ifdef CONFIG_SAE_SUPPORT
void fATWGRP(void *arg)
{

	unsigned char grp_id = 0, i = 0, error = 0;
	int target_grp_id[10] = { 15, 16, 17, 18, 19, 20, 21, 28, 29, 30 };

	if (!arg) {
		error = 1;
	} else {
		grp_id = atoi((const char *) (arg));

		for (i = 0; i < 10; i++)
			if (grp_id == target_grp_id[i])
				break;

		if (i == 10)
			error = 1;
	}

	if (error) {
		printf("[ATGP]error cmd  !!\n\r");
		printf("[ATGP]Usage: ATGP = group_id \n\r");
		printf("      *************************************************\n\r");
		printf("      ECC group: 19, 20, 21, 28, 29, 30 \n\r      DH group: 15, 16, 17, 18\r\n");
		printf("      *************************************************\n\r");
	} else {
		printf("[ATGP]: _AT_WLAN_SET_GRPID [%s]\n\r", (char *) arg);
		wifi_set_group_id(grp_id);
	}

	return;
}
#endif

#ifdef CONFIG_PMKSA_CACHING
void fATWPMK(void *arg)
{

	unsigned char pmk_enable = 0, error = 0;

	if (!arg) {
		error = 1;
	} else {
		if (1 != atoi((const char *) (arg)))
			pmk_enable = 0;
		else
			pmk_enable = 1;

		printf("pmk_enable = %d\r\n", pmk_enable);
		printf("[ATPM]: _AT_WLAN_SET_PMK [%s]\n\r", (char *) arg);
		wifi_set_pmk_cache_enable(pmk_enable);

	}

	if (error) {
		printf("[ATPM]error cmd  !!\n\r");
		printf("[ATPM]Usage: ATPM = enable \n\r");
		printf("      *************************************************\n\r");
		printf("      1: enable; 0: disable \r\n");
		printf("      *************************************************\n\r");
	}

}
#endif
#endif





#elif ATCMD_VER == ATVER_2	// UART module at command

//ATPA=<ssid>,<pwd>,<chl>,<hidden>[,<max_conn>]
void fATPA(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
	int timeout = 20;
	unsigned char hidden_ssid = 0;
	rtw_mode_t wifi_mode_copy;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR,
			   "\r\n[ATPA] Usage: ATPA=<ssid>,<pwd>,<chl>,<hidden>[,<max_conn>]");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if (argc < 5) {
		//at_printf("\r\n[ATPA] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if ((wifi_mode != RTW_MODE_AP) && (wifi_mode != RTW_MODE_STA_AP)) {
		//at_printf("\r\n[ATPA] ERROR : wifi mode error");
		error_no = 5;
		goto exit;
	}
	wifi_mode_copy = wifi_mode;

	//SSID
	if (argv[1] != NULL) {
		ap.ssid.len = strlen((char *) argv[1]);
		if (ap.ssid.len > 32) {
			//at_printf("\r\n[ATPA] ERROR : SSID length can't exceed 32");
			error_no = 2;
			goto exit;
		}
		strcpy((char *) ap.ssid.val, (char *) argv[1]);
	} else {
		//at_printf("\r\n[ATPA] ERROR : SSID can't be empty");
		error_no = 2;
		goto exit;
	}

	//PASSWORD
	if (argv[2] != NULL) {
		if ((strlen(argv[2]) < 8) || (strlen(argv[2]) > 64)) {
			//at_printf("\r\n[ATPA] ERROR : PASSWORD length error");
			error_no = 2;
			goto exit;
		}
		strcpy((char *) ap_pwd_buf, (char *) argv[2]);
		ap.password = ap_pwd_buf;
		ap.password_len = strlen((char *) argv[2]);
		ap.security_type = RTW_SECURITY_WPA2_AES_PSK;
	} else {
		ap.security_type = RTW_SECURITY_OPEN;
	}

	//CHANNEL
	if (argv[3] != NULL) {
		ap.channel = (unsigned char) atoi((const char *) argv[3]);
		if ((ap.channel < 0) || (ap.channel > 11)) {
			//at_printf("\r\n[ATPA] ERROR : channel number error");
			error_no = 2;
			goto exit;
		}
	}
	//HIDDEN SSID
	if (argv[4] != NULL) {
		if ((atoi(argv[4]) != 0) && (atoi(argv[4]) != 1)) {
			//at_printf("\r\n[ATPA] ERROR : HIDDEN SSID must be 0 or 1");
			error_no = 2;
			goto exit;
		}
		hidden_ssid = (unsigned char) atoi((const char *) argv[4]);
	}
	//MAX NUMBER OF STATION
	if (argv[5] != NULL) {
		unsigned char max_sta = atoi(argv[5]);
		if (wext_set_sta_num(max_sta) != 0) {
			error_no = 2;
			goto exit;
		}
	}
#if CONFIG_LWIP_LAYER
	dhcps_deinit();
#endif

	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl);

	wifi_off();
	vTaskDelay(20);

	if (wifi_on(wifi_mode_copy) < 0) {
		//at_printf("\r\n[ATPA] ERROR : Wifi on failed");
		error_no = 3;
		goto exit;
	}

	if (hidden_ssid) {
		if (wifi_start_ap_with_hidden_ssid
		    ((char *) ap.ssid.val, ap.security_type, (char *) ap.password, ap.ssid.len, ap.password_len,
		     ap.channel) < 0) {
			//at_printf("\r\n[ATPA] ERROR : Start AP failed");
			error_no = 4;
			goto exit;
		}
	} else {
		if (wifi_start_ap
		    ((char *) ap.ssid.val, ap.security_type, (char *) ap.password, ap.ssid.len, ap.password_len,
		     ap.channel) < 0) {
			//at_printf("\r\n[ATPA] ERROR : Start AP failed");
			error_no = 4;
			goto exit;
		}
	}

	const char * ifname = wifi_get_ifname(RTW_AP_INTERFACE);
	while (1) {
		char essid[33];
		if (wext_get_ssid(ifname, (uint8_t*)essid) > 0) {
			if (strcmp((char*)essid, (char*)ap.ssid.val) == 0) {
				break;
			}
		}
		if (timeout-- == 0) {
			//at_printf("\r\n[ATPA] ERROR : Start AP timeout");
			error_no = 4;
			break;
		}
		vTaskDelay(1 * configTICK_RATE_HZ);
	}
#if CONFIG_LWIP_LAYER
	struct netif *pn;
	pn = wifi_get_netif(RTW_AP_INTERFACE);

	LwIP_UseStaticIP(pn);
	if (dhcp_mode_ap != DHCP_MODE_DISABLE)
		dhcps_init(pn);
#endif

      exit:
	init_wifi_struct();

	if (error_no == 0)
		at_printf("\r\n[ATPA] OK");
	else
		at_printf("\r\n[ATPA] ERROR:%d", error_no);

	return;
}

/*find ap with "ssid" from scan list*/
static int _find_ap_from_scan_buf(char *buf, int buflen, char *target_ssid, void *user_data)
{
	rtw_wifi_setting_t *pwifi = (rtw_wifi_setting_t *) user_data;
	int plen = 0;

	while (plen < buflen) {
		u8 len, ssid_len, security_mode;
		char *ssid;

		// len offset = 0
		len = (int) *(buf + plen);
		// check end
		if (len == 0)
			break;
		// ssid offset = 14
		ssid_len = len - 14;
		ssid = buf + plen + 14;
		if ((ssid_len == strlen(target_ssid))
		    && (!memcmp(ssid, target_ssid, ssid_len))) {
			strcpy((char *) pwifi->ssid, target_ssid);
			// channel offset = 13
			pwifi->channel = *(buf + plen + 13);
			// security_mode offset = 11
			security_mode = (u8) * (buf + plen + 11);
			if (security_mode == IW_ENCODE_ALG_NONE)
				pwifi->security_type = RTW_SECURITY_OPEN;
			else if (security_mode == IW_ENCODE_ALG_WEP)
				pwifi->security_type = RTW_SECURITY_WEP_PSK;
			else if (security_mode == IW_ENCODE_ALG_CCMP)
				pwifi->security_type = RTW_SECURITY_WPA2_AES_PSK;
			break;
		}
		plen += len;
	}
	return 0;
}

/*get ap security mode from scan list*/
static int _get_ap_security_mode(IN char *ssid, OUT rtw_security_t * security_mode, OUT u8 * channel)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1000;

	memset(&wifi, 0, sizeof(wifi));

	if (wifi_scan_networks_with_ssid(_find_ap_from_scan_buf, (void *) &wifi, scan_buflen, ssid, strlen(ssid)) !=
	    RTW_SUCCESS) {
		printf("Wifi scan failed!\n");
		return 0;
	}

	if (strcmp((char*)wifi.ssid, ssid) == 0) {
		*security_mode = wifi.security_type;
		*channel = wifi.channel;
		return 1;
	}

	return 0;
}

//ATPN=<ssid>,<pwd>[,<key_id>,<bssid>]
static void atcmd_wifi_disconn_hdl(char *buf, int buf_len, int flags, void *userdata)
{
	(void) buf;
	(void) buf_len;
	(void) flags;
	(void) userdata;

#if CONFIG_LOG_SERVICE_LOCK
	log_service_lock();
#endif
	at_printf("\r\n[ATWD] OK");
	at_printf(STR_END_OF_ATCMD_RET);
#if CONFIG_LOG_SERVICE_LOCK
	log_service_unlock();
#endif
}

void fATPN(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };

	int mode, ret;
	char assoc_by_bssid = 0;
	u8 connect_channel;
	u8 pscan_config;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPN] Usage : ATPN=<ssid>,<pwd>[,<key_id>,<bssid>]");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if ((argc < 2) || (argc > 5)) {
		//at_printf("\r\n[ATPN] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if ((wifi_mode != RTW_MODE_STA) && (wifi_mode != RTW_MODE_STA_AP)) {
		//at_printf("\r\n[ATPN] ERROR : wifi mode error");
		error_no = 5;
		goto exit;
	}
	//SSID
	if (argv[1] != NULL) {
		strcpy((char *) wifi.ssid.val, (char *) argv[1]);
		wifi.ssid.len = strlen((char *) argv[1]);
	} else {
		//at_printf("\r\n[ATPN] ERROR : SSID can't be Empty");
		error_no = 2;
		goto exit;
	}
	wifi.security_type = RTW_SECURITY_OPEN;

	//PASSWORD
	if (argv[2] != NULL) {
		int pwd_len = strlen(argv[2]);
		if (pwd_len > 64 || (pwd_len < 8 && pwd_len != 5)) {
			//at_printf("\r\n[ATPN] ERROR : PASSWORD format error");
			error_no = 2;
			goto exit;
		}
		strcpy((char *) password, (char *) argv[2]);
		wifi.password = password;
		wifi.password_len = strlen((char *) argv[2]);
		wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
	}
	//KEYID
	if (argv[3] != NULL) {
		if ((strlen((const char *) argv[3]) != 1) || (*(char *) argv[3] < '0' || *(char *) argv[3] > '3')) {
			//at_printf("\r\n[ATPN] ERROR : Wrong WEP key id. Must be one of 0,1,2, or 3");
			error_no = 2;
			goto exit;
		}
		wifi.key_id = atoi((const char *) (argv[3]));
		wifi.security_type = RTW_SECURITY_WEP_PSK;
	}
	//BSSID
	if (argv[4] != NULL) {
		if (at_hex2bin((u8*)wifi.bssid.octet, ETH_ALEN, (u8*)argv[4], strlen(argv[4])) < 0) {
			error_no = 2;
			goto exit;
		}
		assoc_by_bssid = 1;
	}
	//Check if in AP mode
	wext_get_mode(WLAN0_NAME, &mode);
	if (mode == IW_MODE_MASTER) {
#if CONFIG_LWIP_LAYER
		dhcps_deinit();
#endif
		wifi_off();
		vTaskDelay(20);
		if (wifi_on(RTW_MODE_STA) < 0) {
			//at_printf("\r\n[ATPN] ERROR: Wifi on failed");
			error_no = 3;
			goto exit;
		}
	}
#if 1
	/************************************************************
	*    Get security mode from scan list, if it's WEP and key_id isn't set by user,
	*    system will use default key_id = 0
	************************************************************/
	//the keyID may be not set for WEP which may be confued with WPA2 
	if ((wifi.security_type == RTW_SECURITY_UNKNOWN) || (wifi.security_type == RTW_SECURITY_WPA2_AES_PSK)) {
		int security_retry_count = 0;
		while (1) {
			if (_get_ap_security_mode((char *) wifi.ssid.val, &wifi.security_type, &connect_channel))
				break;
			security_retry_count++;
			if (security_retry_count >= 3) {
				printf("Can't get AP security mode and channel.\n");
				error_no = 6;
				goto exit;
			}
		}
		if (wifi.security_type == RTW_SECURITY_WEP_PSK || wifi.security_type == RTW_SECURITY_WEP_SHARED)
			wifi.key_id = (wifi.key_id < 0 || wifi.key_id > 3) ? 0 : wifi.key_id;
	}
	pscan_config = PSCAN_ENABLE;
	if (connect_channel > 0 && connect_channel < 14)
		wifi_set_pscan_chan(&connect_channel, &pscan_config, 1);
#endif

	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl);
	if (assoc_by_bssid) {
		ret =
		    wifi_connect_bssid(wifi.bssid.octet, (char *) wifi.ssid.val, wifi.security_type,
				       (char *) wifi.password, ETH_ALEN, wifi.ssid.len, wifi.password_len, wifi.key_id,
				       NULL);
	} else {
		ret = wifi_connect((char *) wifi.ssid.val, wifi.security_type, (char *) wifi.password, wifi.ssid.len,
				   wifi.password_len, wifi.key_id, NULL);
	}

	if (ret != RTW_SUCCESS) {
		//at_printf("\r\n[ATPN] ERROR: Can't connect to AP");
		error_no = 4;
		goto exit;
	}
#if CONFIG_LWIP_LAYER
	if (dhcp_mode_sta == DHCP_MODE_AS_SERVER) {
		struct netif *pnetif = &xnetif[0];
		LwIP_UseStaticIP(pnetif);
		dhcps_init(pnetif);
	} else {
		ret = LwIP_DHCP(0, DHCP_START);
		if (ret != DHCP_ADDRESS_ASSIGNED)
			error_no = 7;
	}
#endif


      exit:
	init_wifi_struct();
	if (error_no == 0) {
		wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl, NULL);
		at_printf("\r\n[ATPN] OK");
	} else
		at_printf("\r\n[ATPN] ERROR:%d", error_no);

	return;
}

//ATPH=<mode>,<enable>
void fATPH(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
	int mode, enable;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPH] Usage : ATPH=<mode>,<enable>");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if (argc != 3) {
		//at_printf("\r\n[ATPH] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
		mode = atoi((const char *) (argv[1]));
		if (mode != 1 && mode != 2) {
			//at_printf("\r\n[ATPH] ERROR : parameter must be 1 or 2");
			error_no = 2;
			goto exit;
		}
	}

	if (argv[2] != NULL) {
		enable = atoi((const char *) (argv[2]));
		if (enable != 1 && enable != 2) {
			//at_printf("\r\n[ATPH] ERROR : parameter must be 1 or 2");
			error_no = 2;
			goto exit;
		}
		if (mode == 1)
			dhcp_mode_ap = enable;
		else if (mode == 2)
			dhcp_mode_sta = enable;
	}

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPH] OK");
	else
		at_printf("\r\n[ATPH] ERROR:%d", error_no);

	return;

}

//ATPE=<ip>(,<gateway>,<mask>)
void fATPE(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
	unsigned int ip_addr = 0;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPE] Usage : ATPE=<ip>(,<gateway>,<mask>)");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if ((argc > 4) || (argc < 2)) {
		//at_printf("\r\n[ATPE] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
		ip_addr = inet_addr(argv[1]);
		memcpy(sta_ip, &ip_addr, sizeof sta_ip);
	} else {
		//at_printf("\r\n[ATPE] ERROR : parameter format error");
		error_no = 2;
		goto exit;
	}

	if (argv[2] != NULL) {
		ip_addr = inet_addr(argv[2]);
		memcpy(sta_gw, &ip_addr, sizeof sta_gw);
	} else {
		memcpy(sta_gw, &sta_ip, sizeof sta_gw);
		sta_gw[3] = 1;
	}

	if (argv[3] != NULL) {
		ip_addr = inet_addr(argv[3]);
		memcpy(sta_netmask, &ip_addr, sizeof sta_netmask);
	} else {
		ip_addr = 0x00FFFFFF;
		memcpy(sta_netmask, &ip_addr, sizeof sta_netmask);
	}

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPE] OK");
	else
		at_printf("\r\n[ATPE] ERROR:%d", error_no);

	return;

}

//ATPF=<start_ip>,<end_ip>,<gateway>
void fATPF(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
	unsigned int ip_addr = 0;
	struct ip_addr start_ip, end_ip;

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR,
			   "\r\n[ATPF] Usage : ATPF=<start_ip>,<end_ip>,<ip>(,<gateway>,<mask>)");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if ((argc != 4)) {
		//at_printf("\r\n[ATPF] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
#if LWIP_VERSION_MAJOR >= 2
		ip_addr_set_ip4_u32(&start_ip, inet_addr(argv[1]));
#else
		start_ip.addr = inet_addr(argv[1]);
#endif
	} else {
		//at_printf("\r\n[ATPF] ERROR : parameter format error");
		error_no = 2;
		goto exit;
	}

	if (argv[2] != NULL) {
#if LWIP_VERSION_MAJOR >= 2
		ip_addr_set_ip4_u32(&end_ip, inet_addr(argv[2]));
#else
		end_ip.addr = inet_addr(argv[2]);
#endif
	} else {
		//at_printf("\r\n[ATPF] ERROR : parameter format error");
		error_no = 2;
		goto exit;
	}

	dhcps_set_addr_pool(1, &start_ip, &end_ip);

	if (argv[3] != NULL) {
		ip_addr = inet_addr(argv[3]);
		memcpy(ap_ip, &ip_addr, sizeof ap_ip);
	} else {
		//at_printf("\r\n[ATPF] ERROR : parameter format error");
		error_no = 2;
		goto exit;
	}

	memcpy(ap_gw, ap_ip, sizeof ap_ip);
	ip_addr = 0x00FFFFFF;
	memcpy(ap_netmask, &ip_addr, sizeof ap_netmask);

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPF] OK");
	else
		at_printf("\r\n[ATPF] ERROR:%d", error_no);

	return;
}

int atcmd_wifi_read_info_from_flash(u8 * read_data, u32 read_len)
{
	atcmd_update_partition_info(AT_PARTITION_WIFI, AT_PARTITION_READ, read_data, read_len);
	return 0;
}

void atcmd_wifi_write_info_to_flash(rtw_wifi_setting_t * setting, int enable)
{
	struct atcmd_wifi_conf *data_to_flash;
	rtw_wifi_setting_t *old_setting;
	u32 channel = 0, write_needed = 0;
	u8 index = 0;

	data_to_flash = (struct atcmd_wifi_conf *) malloc(sizeof(struct atcmd_wifi_conf));

	if (data_to_flash) {
		if (enable) {
			memset((u8 *) data_to_flash, 0, sizeof(struct atcmd_wifi_conf));
			atcmd_update_partition_info(AT_PARTITION_WIFI, AT_PARTITION_READ, (u8 *) data_to_flash,
						    sizeof(struct atcmd_wifi_conf));
			old_setting = &(data_to_flash->setting);
			if (memcmp((u8 *) old_setting, setting, sizeof(rtw_wifi_setting_t))) {
				memcpy(old_setting, setting, sizeof(rtw_wifi_setting_t));
				write_needed = 1;
			}
			if (setting->mode == RTW_MODE_STA || setting->mode == RTW_MODE_STA_AP) {
				struct wlan_fast_reconnect reconn;
				int found = 0;
				/*clean wifi ssid,key and bssid */
				memset((u8 *) & reconn, 0, sizeof(struct wlan_fast_reconnect));

				channel = setting->channel;

				memset(psk_essid[index], 0, sizeof(psk_essid[index]));
				strncpy((char*)psk_essid[index], (char*)setting->ssid, strlen((char*)setting->ssid));
				switch (setting->security_type) {
				case RTW_SECURITY_OPEN:
					memset(psk_passphrase[index], 0, sizeof(psk_passphrase[index]));
					memset(wpa_global_PSK[index], 0, sizeof(wpa_global_PSK[index]));
					reconn.security_type = RTW_SECURITY_OPEN;
					break;
				case RTW_SECURITY_WEP_PSK:
					channel |= (setting->key_idx) << 28;
					memset(psk_passphrase[index], 0, sizeof(psk_passphrase[index]));
					memset(wpa_global_PSK[index], 0, sizeof(wpa_global_PSK[index]));
					memcpy(psk_passphrase[index], setting->password, sizeof(psk_passphrase[index]));
					reconn.security_type = RTW_SECURITY_WEP_PSK;
					break;
				case RTW_SECURITY_WPA_TKIP_PSK:
					reconn.security_type = RTW_SECURITY_WPA_TKIP_PSK;
					break;
				case RTW_SECURITY_WPA2_AES_PSK:
					reconn.security_type = RTW_SECURITY_WPA2_AES_PSK;
					break;
				default:
					break;
				}

				memcpy(reconn.psk_essid, psk_essid[index], sizeof(reconn.psk_essid));
				if (strlen((char*)psk_passphrase64) == 64) {
					memcpy(reconn.psk_passphrase, psk_passphrase64, sizeof(reconn.psk_passphrase));
				} else {
					memcpy(reconn.psk_passphrase, psk_passphrase[index],
					       sizeof(reconn.psk_passphrase));
				}
				memcpy(reconn.wpa_global_PSK, wpa_global_PSK[index], sizeof(reconn.wpa_global_PSK));
				memcpy(&(reconn.channel), &channel, 4);

				if (data_to_flash->reconn_num < 0
				    || data_to_flash->reconn_num > ATCMD_WIFI_CONN_STORE_MAX_NUM
				    || data_to_flash->reconn_last_index < 0
				    || data_to_flash->reconn_last_index > ATCMD_WIFI_CONN_STORE_MAX_NUM) {
					data_to_flash->reconn_num = 0;
					data_to_flash->reconn_last_index = -1;
				}

				reconn.enable = enable;
				int i;
				for (i = 0; i < data_to_flash->reconn_num; i++) {
					if (memcmp
					    ((u8 *) & reconn, (u8 *) & (data_to_flash->reconn[i]),
					     sizeof(struct wlan_fast_reconnect)) == 0) {
						AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ALWAYS,
							   "the same profile found in flash");
						found = 1;
						break;
					}
				}
				if (!found) {
					data_to_flash->reconn_last_index++;
					if (data_to_flash->reconn_last_index >= ATCMD_WIFI_CONN_STORE_MAX_NUM)
						data_to_flash->reconn_last_index -= ATCMD_WIFI_CONN_STORE_MAX_NUM;
					memcpy((u8 *) & data_to_flash->reconn[data_to_flash->reconn_last_index],
					       (u8 *) & reconn, sizeof(struct wlan_fast_reconnect));
					data_to_flash->reconn_num++;
					if (data_to_flash->reconn_num > ATCMD_WIFI_CONN_STORE_MAX_NUM)
						data_to_flash->reconn_num = ATCMD_WIFI_CONN_STORE_MAX_NUM;
					write_needed = 1;
				}
			}
			if (write_needed || data_to_flash->auto_enable != enable) {
				data_to_flash->auto_enable = enable;
				atcmd_update_partition_info(AT_PARTITION_WIFI, AT_PARTITION_WRITE, (u8 *) data_to_flash,
							    sizeof(struct atcmd_wifi_conf));
			}
		} else {
			atcmd_update_partition_info(AT_PARTITION_WIFI, AT_PARTITION_ERASE, (u8 *) data_to_flash,
						    sizeof(struct atcmd_wifi_conf));
		}
	}
	if (data_to_flash) {
		free(data_to_flash);
	}
}

int atcmd_wifi_restore_from_flash(void)
{
	struct atcmd_wifi_conf *data;
	rtw_wifi_setting_t *setting;
	struct wlan_fast_reconnect *reconn;
	uint32_t channel;
	uint32_t security_type;
	uint8_t pscan_config;
	char key_id[2] = { 0 };
	int ret = -1, i;
	int mode;
	rtw_network_info_t wifi = {
		{0},		// ssid
		{0},		// bssid
		0,		// security
		NULL,		// password
		0,		// password len
		-1		// key id
	};

	data = (struct atcmd_wifi_conf *) rtw_zmalloc(sizeof(struct atcmd_wifi_conf));
	if (data) {
		atcmd_update_partition_info(AT_PARTITION_WIFI, AT_PARTITION_READ, (u8 *) data,
					    sizeof(struct atcmd_wifi_conf));
		if (data->auto_enable != 1)
			goto exit;
		setting = &data->setting;
		if (setting->mode == RTW_MODE_AP || setting->mode == RTW_MODE_STA_AP) {
			//start AP here
			goto exit;
		}
		//Check if in AP mode
		wext_get_mode(WLAN0_NAME, &mode);
		if (mode == IW_MODE_MASTER) {
#if CONFIG_LWIP_LAYER
			dhcps_deinit();
#endif
			wifi_off();
			vTaskDelay(20);
			if (wifi_on(RTW_MODE_STA) < 0) {
				printf("\n\rERROR: Wifi on failed!");
				ret = -1;
				goto exit;
			}
		}
#if CONFIG_AUTO_RECONNECT
		//setup reconnection flag
		wifi_set_autoreconnect(0);
#endif
		int last_index = data->reconn_last_index;
		for (i = 0; i < data->reconn_num; i++) {
			reconn = &data->reconn[last_index];
			last_index++;
			if (last_index >= ATCMD_WIFI_CONN_STORE_MAX_NUM)
				last_index -= ATCMD_WIFI_CONN_STORE_MAX_NUM;
			if (reconn->enable != 1) {
				continue;
			}
			memcpy(psk_essid, reconn->psk_essid, sizeof(reconn->psk_essid));
			memcpy(psk_passphrase, reconn->psk_passphrase, sizeof(reconn->psk_passphrase));
			memcpy(wpa_global_PSK, reconn->wpa_global_PSK, sizeof(reconn->wpa_global_PSK));
			channel = reconn->channel;
			sprintf(key_id, "%d", (char) (channel >> 28));
			channel &= 0xff;
			security_type = reconn->security_type;
			pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
			//set partial scan for entering to listen beacon quickly
			wifi_set_pscan_chan((uint8_t *) & channel, &pscan_config, 1);

			wifi.security_type = security_type;
			//SSID
			strcpy((char *) wifi.ssid.val, (char *) psk_essid);
			wifi.ssid.len = strlen((char *) psk_essid);

			switch (security_type) {
			case RTW_SECURITY_WEP_PSK:
				wifi.password = (unsigned char *) psk_passphrase;
				wifi.password_len = strlen((char *) psk_passphrase);
				wifi.key_id = atoi((const char *) key_id);
				break;
			case RTW_SECURITY_WPA_TKIP_PSK:
			case RTW_SECURITY_WPA2_AES_PSK:
				wifi.password = (unsigned char *) psk_passphrase;
				wifi.password_len = strlen((char *) psk_passphrase);
				break;
			default:
				break;
			}

			ret =
			    wifi_connect((char *) wifi.ssid.val, wifi.security_type, (char *) wifi.password,
					 wifi.ssid.len, wifi.password_len, wifi.key_id, NULL);
			if (ret == RTW_SUCCESS) {
				LwIP_DHCP(0, DHCP_START);
				ret = 0;
				break;
			}
		}
	}

      exit:
	if (ret == 0)
		wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl, NULL);
	if (data)
		rtw_mfree((u8 *) data, sizeof(struct wlan_fast_reconnect));
	return ret;
}

//ATPG=<enable>
void fATPG(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };
//      flash_t flash;
//      struct wlan_fast_reconnect read_data = {0};

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPG] Usage : ATPG=<enable>");
		error_no = 1;
		goto exit;
	}
	argc = parse_param(arg, argv);
	if (argc != 2) {
		//at_printf("\r\n[ATPG] ERROR : command format error");
		error_no = 1;
		goto exit;
	}
	//ENABLE FAST CONNECT
	if (argv[1] != NULL) {
#if 0
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect), (u8 *) & read_data);
		read_data.enable = atoi((const char *) (argv[1]));
		if (read_data.enable != 0 && read_data.enable != 1) {
			//at_printf("\r\n[ATPG] ERROR : parameter must be 0 or 1");
			error_no = 2;
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			goto exit;
		}
		flash_erase_sector(&flash, FAST_RECONNECT_DATA);
		flash_stream_write(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect), (u8 *) & read_data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
#else
		rtw_wifi_setting_t setting;
		int enable = atoi((const char *) (argv[1]));
		if (enable != 0 && enable != 1) {
			error_no = 2;
			goto exit;
		}
		if (enable == 1) {
			u8 *ifname[1] = { (u8*)WLAN0_NAME };
			if (wifi_get_setting((const char *) ifname[0], &setting)) {
				AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "wifi_get_setting fail");
				error_no = 3;
				goto exit;
			}
		}
		atcmd_wifi_write_info_to_flash(&setting, enable);
#endif
	}

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPG] OK");
	else
		at_printf("\r\n[ATPG] ERROR:%d", error_no);

	return;
}

//ATPM=<mac>
void fATPM(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPM] Usage : ATPM=<mac>");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if (argc != 2) {
		//at_printf("\r\n[ATPM] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
		if (strlen(argv[1]) != 12) {
			//at_printf("\r\n[ATPM] ERROR : BSSID format error");
			error_no = 2;
			goto exit;
		}
		wifi_set_mac_address(argv[1]);
	}

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPM] OK");
	else
		at_printf("\r\n[ATPM] ERROR:%d", error_no);

	return;

}

//ATPW=<mode>
void fATPW(void *arg)
{
	int argc, error_no = 0;
	char *argv[MAX_ARGC] = { 0 };

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_WIFI, AT_DBG_ERROR, "\r\n[ATPW] Usage : ATPW=<mode>");
		error_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);
	if (argc != 2) {
		//at_printf("\r\n[ATPW] ERROR : command format error");
		error_no = 1;
		goto exit;
	}

	if (argv[1] != NULL) {
		wifi_mode = atoi((const char *) (argv[1]));
		if ((wifi_mode != RTW_MODE_STA) && (wifi_mode != RTW_MODE_AP) && (wifi_mode != RTW_MODE_STA_AP)) {
			//at_printf("\r\n[ATPW] ERROR : parameter must be 1 , 2 or 3");
			error_no = 2;
		}
	}

      exit:
	if (error_no == 0)
		at_printf("\r\n[ATPW] OK");
	else
		at_printf("\r\n[ATPW] ERROR:%d", error_no);

	return;
}

void print_wlan_help(void *arg)
{
	(void) arg;
	at_printf("\r\nWLAN AT COMMAND SET:");
	at_printf("\r\n==============================");
	at_printf("\r\n1. Wlan Scan for Network Access Point");
	at_printf("\r\n   # ATWS");
	at_printf("\r\n2. Connect to an AES AP");
	at_printf("\r\n   # ATPN=<ssid>,<pwd>,<key_id>(,<bssid>)");
	at_printf("\r\n3. Create an AES AP");
	at_printf("\r\n   # ATPA=<ssid>,<pwd>,<chl>,<hidden>");
}

#endif// end of #if ATCMD_VER == ATVER_1

#endif// end of #if CONFIG_WLAN





#if CONFIG_LWIP_LAYER
#if ATCMD_VER == ATVER_1
void fATWL(void *arg)
{
	/* To avoid gcc warnings */
	(void) arg;

	#if CONFIG_SSL_CLIENT
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	printf("[ATWL]: _AT_WLAN_SSL_CLIENT_\n\r");
	argv[0] = "ssl_client";
	if (!arg) {
		printf("ATWL=SSL_SERVER_HOST\n\r");
		return;
	}
	if ((argc = parse_param(arg, argv)) > 1) {
		if (argc != 2) {
			printf("ATWL=SSL_SERVER_HOST\n\r");
			return;
		}

		cmd_ssl_client(argc, argv);
	}
	#else
	printf("Please set CONFIG_SSL_CLIENT 1 in platform_opts.h to enable ATWL command\n");
	#endif
}

void fATWI(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	printf("[ATWI]: _AT_WLAN_PING_TEST_\n\r");

	if (!arg) {
		printf("\n\r[ATWI] Usage: ATWI=[host],[options]\n");
		printf("\n\r     -t        Ping the specified host until stopped\n");
		printf("  \r     -n    #   Number of echo requests to send (default 4 times)\n");
		printf("  \r     -l    #   Send buffer size (default 32 bytes)\n");
		printf("\n\r   Example:\n");
		printf("  \r     ATWI=192.168.1.2,-n,100,-l,5000\n");
		return;
	}

	argv[0] = "ping";

	if ((argc = parse_param(arg, argv)) > 1) {
		cmd_ping(argc, argv);
	}
}

void fATWT(void *arg)
{
	#if CONFIG_BSD_TCP
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	printf("[ATWT]: _AT_WLAN_TCP_TEST_\n\r");

	if (!arg) {
		printf("\n\r[ATWT] Usage: ATWT=[-s|-c,host|stop],[options]\n");
		printf("\n\r   Client/Server:\n");
		printf("  \r     stop           terminate client & server\n");
		printf("  \r     -i    #        seconds between periodic bandwidth reports\n");
		printf("  \r     -l    #        length of buffer to read or write (default 1460 Bytes)\n");
		printf("  \r     -p    #        server port to listen on/connect to (default 5001)\n");
		printf("\n\r   Server specific:\n");
		printf("  \r     -s             run in server mode\n");
		printf("\n\r   Client specific:\n");
		printf("  \r     -c    <host>   run in client mode, connecting to <host>\n");
		printf("  \r     -d             do a bidirectional test simultaneously\n");
		printf("  \r     -t    #        time in seconds to transmit for (default 10 secs)\n");
		printf("  \r     -n    #[KM]    number of bytes to transmit (instead of -t)\n");
		printf("\n\r   Example:\n");
		printf("  \r     ATWT=-s,-p,5002\n");
		printf("  \r     ATWT=-c,192.168.1.2,-t,100,-p,5002\n");
		return;
	}

	argv[0] = "tcp";

	if ((argc = parse_param(arg, argv)) > 1) {
		cmd_tcp(argc, argv);
	}
	#else
	printf("Please set CONFIG_BSD_TCP 1 in platform_opts.h to enable ATWT command\n");
	#endif
}

void fATWU(void *arg)
{
	#if CONFIG_BSD_TCP
	int argc;
	char *argv[MAX_ARGC] = { 0 };

	printf("[ATWU]: _AT_WLAN_UDP_TEST_\n\r");

	if (!arg) {
		printf("\n\r[ATWU] Usage: ATWU=[-s|-c,host|stop][options]\n");
		printf("\n\r   Client/Server:\n");
		printf("  \r     stop           terminate client & server\n");
		printf("  \r     -i    #        seconds between periodic bandwidth reports\n");
		printf("  \r     -l    #        length of buffer to read or write (default 1460 Bytes)\n");
		printf("  \r     -p    #        server port to listen on/connect to (default 5001)\n");
		printf("\n\r   Server specific:\n");
		printf("  \r     -s             run in server mode\n");
		printf("\n\r   Client specific:\n");
		printf("  \r     -b    #[KM]    for UDP, bandwidth to send at in bits/sec (default 1 Mbit/sec)\n");
		printf("  \r     -c    <host>   run in client mode, connecting to <host>\n");
		printf("  \r     -d             do a bidirectional test simultaneously\n");
		printf("  \r     -t    #        time in seconds to transmit for (default 10 secs)\n");
		printf("  \r     -n    #[KM]    number of bytes to transmit (instead of -t)\n");
		printf("  \r     -S    #        set the IP 'type of service'\n");
		printf("\n\r   Example:\n");
		printf("  \r     ATWU=-s,-p,5002\n");
		printf("  \r     ATWU=-c,192.168.1.2,-t,100,-p,5002\n");
		return;
	}

	argv[0] = "udp";

	if ((argc = parse_param(arg, argv)) > 1) {
		cmd_udp(argc, argv);
	}
	#else
	printf("Please set CONFIG_BSD_TCP 1 in platform_opts.h to enable ATWU command\n");
	#endif
}

#elif ATCMD_VER == ATVER_2	// uart at command
//move to atcmd_lwip.c
#endif//ATCMD_VER
#endif//CONFIG_LWIP_LAYER




/* Set the WiFi Mode (Station, AP, Station+AP) */
void fATCWMODE(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	int mode;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	// Query
	if (*argv[1] == '?') {
		at_printf("+CWMODE:%d\r\n", wifi_mode);
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	mode = atoi(argv[1]);
	if (mode != RTW_MODE_STA && mode != RTW_MODE_AP && mode != RTW_MODE_STA_AP) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	if (wifi_set_mode(mode) != RTW_SUCCESS) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	at_printf(STR_RESP_OK);
	return;
}

/* Enables or Disables DHCP */
void fATCWDHCP(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	int mode, enable;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	// Query
	if (*argv[1] == '?') {
		mode  = dhcp_mode_sta != DHCP_MODE_DISABLE;
		mode |= (dhcp_mode_ap != DHCP_MODE_DISABLE) << 1;
		at_printf("+CWDHCP:%d\r\n", mode);
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	enable = atoi(argv[1]);
	if (enable > 1 || argv[2] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	mode = atoi(argv[2]);

	if (enable) {
		if (mode & 0x1)
			dhcp_mode_sta = DHCP_MODE_ENABLE;
		if (mode & 0x2)
			dhcp_mode_ap  = DHCP_MODE_ENABLE;
	} else {
		if (mode & 0x1)
			dhcp_mode_sta = DHCP_MODE_DISABLE;
		if (mode & 0x2)
			dhcp_mode_ap  = DHCP_MODE_DISABLE;
	}

	at_printf(STR_RESP_OK);
	return;
}

int security_2_esp_ecn(int security) {
	int ecn;

	switch (security) {
	case RTW_SECURITY_OPEN:
		ecn = 0; break;
	case RTW_SECURITY_WEP_PSK:
		ecn = 1; break;
	case RTW_SECURITY_WPA_TKIP_PSK:
	case RTW_SECURITY_WPA_AES_PSK:
		ecn = 2; break;
	case RTW_SECURITY_WPA2_AES_PSK:
	case RTW_SECURITY_WPA2_TKIP_PSK:
	case RTW_SECURITY_WPA2_MIXED_PSK:
		ecn = 3; break;
	case RTW_SECURITY_WPA_WPA2_MIXED:
		ecn = 4; break;
	default:
		/* TODO: Enterprise or Unkown ? */
		ecn = 5; break;
	}
	return ecn;

}

int esp_ecn_2_security(int ecn) {
	int security;

	// security = RTW_SECURITY_UNKNOWN;
	security = RTW_SECURITY_UNKNOWN;
	switch (ecn) {
	case 0:
		security = RTW_SECURITY_OPEN; break;
	case 2:
		security = RTW_SECURITY_WPA_AES_PSK; break;
	case 4:
		security = RTW_SECURITY_WPA_WPA2_MIXED; break;
	case 3:
	default:
		security = RTW_SECURITY_WPA2_AES_PSK; break;
	}
	return security;
}

static rtw_result_t cwlap_result_handler2(rtw_scan_handler_result_t * result)
{
	int ecn;

	if (result->scan_complete != RTW_TRUE) {
		rtw_scan_result_t *record = &result->ap_details;
		record->SSID.val[record->SSID.len] = 0;	/* Ensure the SSID is null terminated */
		ecn = security_2_esp_ecn(record->security);

		at_printf("+CWLAP:(%d,\"%s\",%d,\"" MAC_FMT "\",%d)\r\n",
				ecn,
				record->SSID.val,
				record->signal_strength,
				MAC_ARG(record->BSSID.octet),
				record->channel);
	} else {
		at_printf(STR_RESP_OK);
	}
	return RTW_SUCCESS;
}



/* List the Available APs */
void fATCWLAP(void* arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = { 0 };
	int err = RTW_SUCCESS;

	if (arg) {
		argc = parse_param(arg, argv);
		if (argc < 2 || argv[1] == NULL) {
			at_printf(STR_RESP_FAIL);
			return;
		}
	}

	if ((err = wifi_scan_networks(cwlap_result_handler2, NULL)) != RTW_SUCCESS) {
		printf("+CWLAP: scan error %d\n\r", err);
		at_printf(STR_RESP_FAIL);
		return;
	}

	return;
}


static void cwqap_wifi_disconn_handler(char *buf, int buf_len, int flags, void *userdata)
{
	(void) buf;
	(void) buf_len;
	(void) flags;
	(void) userdata;

	at_printf(STR_RESP_OK);
	at_printf("\r\nWIFI DISCONNECT\r\n");
	at_set_ipstatus(ESP_IPSTAT_NO_AP);
	return;
}

/* Disconnect from the AP */
void fATCWQAP(void* arg) {
	char essid[33];
	int timeout;
	int err = RTW_SUCCESS;

	(void) arg;

	if (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
		at_printf(STR_RESP_OK);
		return;
	}

	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, cwqap_wifi_disconn_handler);

	if ((err = wifi_disconnect()) < 0) {
		goto __ret;
	}

	for (timeout = 8; timeout; timeout--) {
		if (wext_get_ssid(WLAN0_NAME, (uint8_t*) essid) < 0) {
			break;
		}

		vTaskDelay(1 * configTICK_RATE_HZ);
	}

	if (timeout <= 0) {
		err = RTW_TIMEOUT;
	}
	LwIP_ReleaseIP(WLAN0_IDX);

__ret:
	init_wifi_struct();
	if (err == RTW_SUCCESS) {
		at_printf(STR_RESP_OK);
		return;
	}

	printf("+CWQAP: ERROR %d\r\n", err);
	at_printf(STR_RESP_FAIL);
	return;
}

/* Connect to the AP */
void fATCWJAP(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	rtw_wifi_setting_t setting[1];
	uint8_t bssid[BSSID_LEN] = { 0 };
	const char* ifname;
	int r = 0;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	ifname = wifi_get_ifname(RTW_STA_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		int rssi;

		r = wifi_get_setting(ifname, setting);
		(void) r;

		wifi_get_rssi(&rssi);

		wifi_get_ap_bssid(bssid);
		at_printf("+CWJAP:\"%s\",\"" MAC_FMT "\",%d,%d\r\n",
				setting->ssid, MAC_ARG(bssid), setting->channel, rssi);
		at_printf(STR_RESP_OK);
		return;
	}

	// Connect action
	if ((wifi_mode != RTW_MODE_STA) && (wifi_mode != RTW_MODE_STA_AP)) {
		r = -1;
		goto __ret;
	}

	//SSID
	strcpy((char *) wifi.ssid.val, (char *)argv[1]);
	wifi.ssid.len = strlen((char *)argv[1]);
	wifi.security_type = RTW_SECURITY_OPEN;

	//PASSWORD
	if (argv[2] != NULL) {
		int pwd_len = strlen(argv[2]);
		if (pwd_len > 64 || (pwd_len < 8 && pwd_len != 5)) {
			printf("+CWJAP: PASSWORD format error\r\n");
			r = -2;
			goto __ret;
		}
		strcpy((char *) password, (char *) argv[2]);
		wifi.password = password;
		wifi.password_len = strlen((char *) argv[2]);
		wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
	}
	//KEYID
	wifi.key_id = 0;
	// wifi.security_type = RTW_SECURITY_WEP_PSK;

	//BSSID
	if (argv[3] != NULL) {
		if (at_hex2bin((u8*)wifi.bssid.octet, ETH_ALEN, (u8*)argv[3], strlen(argv[3])) < 0) {
			memset(wifi.bssid.octet, '\0', sizeof wifi.bssid.octet);
			printf("+CWJAP: BSSID format error");
			r = -3;
			goto __ret;
		}
	}

	extern void task_connect_to_ap(void *param);
	#if 1
	/* Asynchronized the connection process */
	r = xTaskCreate(task_connect_to_ap, "conn-to-ap", 1024, NULL, tskIDLE_PRIORITY + 5, NULL);
	if (r == pdPASS) {
		r = 0;
	} else {
		r = -2000 + r;
	}
	#else
	/* Synchronized connection process */
	task_connect_to_ap(&r);
	#endif

__ret:
	if (r) {
		printf("+CWJAP: ERROR %d\r\n", r);
		at_printf(STR_RESP_FAIL);
		return;
	}
	at_printf(STR_RESP_OK);
	return;
}

void task_connect_to_ap(void *param) {
	int r = 0;

	/* To avoid gcc warnings */
	(void) param;

	char assoc_by_bssid = 0;
	if (memcmp(wifi.bssid.octet, "\0\0\0\0\0\0", sizeof wifi.bssid.octet)) {
		assoc_by_bssid = 1;
	}

	// MODE must already changed by AT+CWMODE
	#if 0
	//Check if in AP mode
	int mode;
	wext_get_mode(ifname, &mode);
	if (mode == IW_MODE_MASTER) {
		dhcps_deinit();
		wifi_off();
		vTaskDelay(20);
		if (wifi_on(RTW_MODE_STA) < 0) {
			r = -4;
			goto __ret;
		}
	}
	#endif

	u8 connect_channel;
	/************************************************************
	*    Get security mode from scan list, if it's WEP and key_id isn't set by user,
	*    system will use default key_id = 0
	************************************************************/
	//the keyID may be not set for WEP which may be confued with WPA2
	if ((wifi.security_type == RTW_SECURITY_UNKNOWN) || (wifi.security_type == RTW_SECURITY_WPA2_AES_PSK)) {
		int security_retry_count = 0;
		while (1) {
			if (_get_ap_security_mode((char *) wifi.ssid.val, &wifi.security_type, &connect_channel))
				break;
			security_retry_count++;
			if (security_retry_count >= 3) {
				printf("Can't get AP security mode and channel.\n");
				r = -5;
				goto __ret;
			}
		}
		if (wifi.security_type == RTW_SECURITY_WEP_PSK || wifi.security_type == RTW_SECURITY_WEP_SHARED)
			wifi.key_id = (wifi.key_id < 0 || wifi.key_id > 3) ? 0 : wifi.key_id;
	}

	u8 pscan_config = PSCAN_ENABLE;
	if (connect_channel > 0 && connect_channel < 14)
		wifi_set_pscan_chan(&connect_channel, &pscan_config, 1);

	int ret;
	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, cwqap_wifi_disconn_handler);
	if (assoc_by_bssid) {
		ret = wifi_connect_bssid(wifi.bssid.octet, (char *) wifi.ssid.val, wifi.security_type,
				        (char *) wifi.password, ETH_ALEN, wifi.ssid.len, wifi.password_len, wifi.key_id,
				        NULL);
	} else {
		ret = wifi_connect((char *) wifi.ssid.val, wifi.security_type, (char *) wifi.password, wifi.ssid.len,
				   wifi.password_len, wifi.key_id, NULL);
	}

	if (ret != RTW_SUCCESS) {
		r = -1000 + ret;
		goto __ret;
	}

	// esp compatible
	at_printf("\r\nWIFI CONNECTED\r\n");

	if (dhcp_mode_sta == DHCP_MODE_AS_CLIENT) {
		ret = LwIP_DHCP(0, DHCP_START);
		if (ret != DHCP_ADDRESS_ASSIGNED) {
			r = -2000 + ret;
			goto __ret;
		}
	} else {
		struct netif *pn;
		pn = wifi_get_netif(RTW_STA_INTERFACE);

		LwIP_UseStaticIP(pn);
		if (dhcp_mode_sta == DHCP_MODE_AS_SERVER) {
			dhcps_init(pn);
		}
	}
	// esp compatible
	at_printf("\r\nWIFI GOT IP\r\n");
	at_set_ipstatus(ESP_IPSTAT_AP_AND_IP);

__ret:
	init_wifi_struct();
	if (!r) {
		wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, cwqap_wifi_disconn_handler, NULL);
	}
	if (param) {
		*(int*)param = r;
		return;
	}
	vTaskDelete(NULL);
	return;
}

/* Set/Get Soft AP info */
void fATCWSAP(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	struct netif* pn;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	pn = wifi_get_netif(RTW_AP_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		rtw_bss_info_t bss_info[1];
		rtw_security_t sec;
		uint8_t ap_ssid[MAX_SSID_LEN + 1/*for NULL terminator */];

		if (wifi_get_ap_info(bss_info, &sec) != RTW_SUCCESS) {
			at_printf(STR_RESP_FAIL);
			return;
		}

		memcpy(ap_ssid, bss_info->SSID, bss_info->SSID_len);
		ap_ssid[bss_info->SSID_len] = '\0';
		at_printf("+CWSAP:\"%s\",\"%s\",%d,%d,%d,%d\r\n",
				ap_ssid,
				ap_pwd_buf,
				bss_info->channel,
				security_2_esp_ecn(sec),
				/* TODO */5,
				0);
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	int r = 0, r1;
	int timeout = 20;
	unsigned char hidden_ssid = 0;
	rtw_mode_t wifi_mode_copy;

	if ((wifi_mode != RTW_MODE_AP) && (wifi_mode != RTW_MODE_STA_AP)) {
		r = -1;
		goto __ret;
	}
	wifi_mode_copy = wifi_mode;

	//SSID
	ap.ssid.len = strlen((char *) argv[1]);
	if (ap.ssid.len > MAX_SSID_LEN) {
		r = -2;
		goto __ret;
	}
	strcpy((char *)ap.ssid.val, (char *) argv[1]);

	//PASSWORD
	if (argv[2] != NULL) {
		if ((strlen(argv[2]) < 8) || (strlen(argv[2]) > 64)) {
			r = -4;
			goto __ret;
		}
		strcpy((char *)ap_pwd_buf, (char *)argv[2]);
		ap.password = ap_pwd_buf;
		ap.password_len = strlen((char *)ap_pwd_buf);
		ap.security_type = RTW_SECURITY_WPA2_AES_PSK;
	} else {
		ap_pwd_buf[0] = '\0';
		ap.security_type = RTW_SECURITY_OPEN;
	}

	//CHANNEL
	if (argv[3] != NULL) {
		ap.channel = (uint8_t)atoi((const char *) argv[3]);
		if ((ap.channel < 0) || (ap.channel > 11)) {
			r = -5;
			goto __ret;
		}
	}

	//ECN(encryption type)
	if (argv[4] != NULL) {
		int ecn = atoi((char *)argv[4]);
		ap.security_type = esp_ecn_2_security(ecn);
	}

	//MAX NUMBER OF STATION
	if (argv[5] != NULL) {
		unsigned char max_sta = atoi(argv[5]);
		if (wext_set_sta_num(max_sta) != 0) {
			r = -6;
			goto __ret;
		}
	}

	//HIDDEN SSID
	if (argv[6] != NULL) {
		if ((atoi(argv[6]) != 0) && (atoi(argv[6]) != 1)) {
			r = -7;
			goto __ret;
		}
		hidden_ssid = (uint8_t)atoi((char *) argv[6]);
	}

	dhcps_deinit();

	wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, atcmd_wifi_disconn_hdl);

	wifi_off();
	vTaskDelay(20);

	if (wifi_on(wifi_mode_copy) < 0) {
		r = -8;
		goto __ret;
	}

	if (hidden_ssid) {
		if ((r1 = wifi_start_ap_with_hidden_ssid
		    ((char*)ap.ssid.val, ap.security_type, (char*)ap.password, ap.ssid.len, ap.password_len,
		     ap.channel)) < 0) {
			r = -1000 + r1;
			goto __ret;
		}
	} else {
		if ((r1 = wifi_start_ap
		    ((char*) ap.ssid.val, ap.security_type, (char*)ap.password, ap.ssid.len, ap.password_len,
		     ap.channel)) < 0) {
			r = -2000 + r1;
			goto __ret;
		}
	}

	const char* ifname = wifi_get_ifname(RTW_AP_INTERFACE);
	while (1) {
		char essid[33];
		if (wext_get_ssid(ifname, (uint8_t*)essid) > 0) {
			if (strcmp((char*)essid, (char*)ap.ssid.val) == 0) {
				break;
			}
		}
		if (timeout-- == 0) {
			r = -11;
			goto __ret;
		}
		vTaskDelay(1 * configTICK_RATE_HZ);
	}
	pn = wifi_get_netif(RTW_AP_INTERFACE);

	LwIP_UseStaticIP(pn);
	if (dhcp_mode_ap != DHCP_MODE_DISABLE)
		dhcps_init(pn);

__ret:
	init_wifi_struct();
	if (r >= 0) {
		at_printf(STR_RESP_OK);
		return;
	}

	printf("+CWSAP: ERROR %d\r\n", r);
	at_printf(STR_RESP_FAIL);
	return;
}

/* Set the Configuration for command AT+CWLAP */
void fATCWLAPOPT(void* arg) {
	(void) arg;
	at_printf(STR_RESP_OK);
	return;
}

/* Enable/Disable Multiple Connections */
void fATCIPMUX(void* arg) {
	(void) arg;
	at_printf(STR_RESP_OK);
	return;
}

/* Get the Connection Status */
void fATCIPSTATUS(void* arg) {
	if (arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}
	at_printf("STATUS:%d\r\n", esp_ipstatus);
	at_printf(STR_RESP_OK);
	return;
}

/* Get/Set the IP Address of Station */
void fATCIPSTA(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	struct netif* pn;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	pn = wifi_get_netif(RTW_STA_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		at_printf("+CIPSTA:ip:\"%s\"\r\n",      ip_ntoa(&pn->ip_addr));
		at_printf("+CIPSTA:gateway:\"%s\"\r\n", ip_ntoa(&pn->gw));
		at_printf("+CIPSTA:netmask:\"%s\"\r\n", ip_ntoa(&pn->netmask));
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	inet_aton(argv[1], &pn->ip_addr);
	memcpy(sta_ip, &pn->ip_addr, sizeof sta_ip);
	if (argc >= 3) {
		inet_aton(argv[2], &pn->gw);
		memcpy(sta_gw, &pn->gw, sizeof sta_gw);
	}
	if (argc >= 4) {
		inet_aton(argv[3], &pn->netmask);
		memcpy(sta_netmask, &pn->netmask, sizeof sta_netmask);
	}

	at_printf(STR_RESP_OK);
	return;
}

/* Get/Set the Mac Address of Station */
void fATCIPSTAMAC(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	struct netif* pn;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	pn = wifi_get_netif(RTW_STA_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		u8 *mac = LwIP_GetMAC(pn);

		at_printf("+CIPSTAMAC:\"" MAC_FMT "\"\r\n", MAC_ARG(mac));
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	at_hex2bin(pn->hwaddr, NETIF_MAX_HWADDR_LEN, argv[1], strlen(argv[1]));
	at_printf(STR_RESP_OK);
	return;
}



/* Get/Set the IP Address of AP */
void fATCIPAP(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	struct netif* pn;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	pn = wifi_get_netif(RTW_AP_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		at_printf("+CIPAP:ip:\"%s\"\r\n",      ip_ntoa(&pn->ip_addr));
		at_printf("+CIPAP:gateway:\"%s\"\r\n", ip_ntoa(&pn->gw));
		at_printf("+CIPAP:netmask:\"%s\"\r\n", ip_ntoa(&pn->netmask));
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	inet_aton(argv[1], &pn->ip_addr);
	memcpy(ap_ip, &pn->ip_addr, sizeof ap_ip);
	if (argc >= 3) {
		inet_aton(argv[2], &pn->gw);
		memcpy(ap_gw, &pn->gw, sizeof ap_gw);
	}
	if (argc >= 4) {
		inet_aton(argv[3], &pn->netmask);
		memcpy(ap_netmask, &pn->netmask, sizeof ap_netmask);
	}
	at_printf(STR_RESP_OK);
	return;
}

/* Get/Set the Mac Address of AP */
void fATCIPAPMAC(void* arg) {
	int argc;
	char *argv[MAX_ARGC] = { 0 };
	struct netif* pn;

	if (!arg) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	argc = parse_param(arg, argv);
	if (argc < 2 || argv[1] == NULL) {
		at_printf(STR_RESP_FAIL);
		return;
	}

	pn = wifi_get_netif(RTW_AP_INTERFACE);

	// Query
	if (*argv[1] == '?') {
		u8 *mac = LwIP_GetMAC(pn);

		at_printf("+CIPAPMAC:\"" MAC_FMT "\"\r\n", MAC_ARG(mac));
		at_printf(STR_RESP_OK);
		return;
	}

	// Set
	at_hex2bin(pn->hwaddr, NETIF_MAX_HWADDR_LEN, argv[1], strlen(argv[1]));
	at_printf(STR_RESP_OK);
	return;
}

/* Show the Remote IP & Port with "+IPD" */
void fATCIPDINFO(void* arg) {
	(void) arg;
	at_printf(STR_RESP_OK);
	return;
}





log_item_t at_wifi_items[] = {
#if ATCMD_VER == ATVER_1
#if CONFIG_LWIP_LAYER
	{"ATWL", fATWL, {NULL, NULL}},
	{"ATWI", fATWI, {NULL, NULL}},
	{"ATWT", fATWT, {NULL, NULL}},
	{"ATWU", fATWU, {NULL, NULL}},
#endif
#if WIFI_LOGO_CERTIFICATION_CONFIG
	{"ATPE", fATPE,},	// set static IP for STA
#ifdef CONFIG_SAE_SUPPORT
	{"ATGP", fATWGRP,},	// set SAE group
#endif
#ifdef CONFIG_PMKSA_CACHING
	{"ATPM", fATWPMK,},	// enable pmk
#endif
#endif
#if CONFIG_WLAN
	{"ATW0", fATW0, {NULL, NULL}},
	{"ATW1", fATW1, {NULL, NULL}},
	{"ATW2", fATW2, {NULL, NULL}},
	{"ATW3", fATW3, {NULL, NULL}},
	{"ATW4", fATW4, {NULL, NULL}},
	{"ATW5", fATW5, {NULL, NULL}},
	{"ATW6", fATW6, {NULL, NULL}},
#ifdef CONFIG_FPGA
	{"ATW7", fATW7,},
#endif
	{"ATWA", fATWA, {NULL, NULL}},
#ifdef  CONFIG_CONCURRENT_MODE
	{"ATWB", fATWB, {NULL, NULL}},
#endif
	{"ATWC", fATWC, {NULL, NULL}},
	{"ATWD", fATWD, {NULL, NULL}},
	{"ATWP", fATWP, {NULL, NULL}},
#if CONFIG_WOWLAN_SERVICE
	{"ATWV", fATWV,},
#endif
	{"ATWR", fATWR, {NULL, NULL}},
	{"ATWS", fATWS, {NULL, NULL}},
#ifdef WIFI_PERFORMANCE_MONITOR
	{"ATWm", fATWm,},
#endif
#if SCAN_WITH_SSID
	{"ATWs", fATWs,},
#endif
#ifdef CONFIG_PROMISC
	{"ATWM", fATWM, {NULL, NULL}},
#endif
	{"ATWZ", fATWZ, {NULL, NULL}},
#if CONFIG_OTA_UPDATE
	{"ATWO", fATWO,},
#endif
#if (CONFIG_INCLUDE_SIMPLE_CONFIG)
	{"ATWQ", fATWQ, {NULL, NULL}},
#endif
#if defined(CONFIG_INCLUDE_DPP_CONFIG) && CONFIG_INCLUDE_DPP_CONFIG
	{"ATWq", fATWq,},
#endif
#ifdef CONFIG_WPS
	{"ATWW", fATWW, {NULL, NULL}},
	{"ATWw", fATWw, {NULL, NULL}},	//wps registrar for softap
#if CONFIG_ENABLE_P2P
	{"ATWG", fATWG,},	//p2p start
	{"ATWH", fATWH,},	//p2p stop
	{"ATWJ", fATWJ,},	//p2p connect
	{"ATWK", fATWK,},	//p2p disconnect
	{"ATWN", fATWN,},	//p2p info
	{"ATWF", fATWF,},	//p2p find
	{"ATWg", fATWg,},	//p2p auto go start
#endif
#endif

#if CONFIG_AIRKISS
	{"ATWX", fATWX,},
#endif
	{"ATW?", fATWx, {NULL, NULL}},
	{"ATW+ABC", fATWx, {NULL, NULL}},
#ifdef CONFIG_POWER_SAVING
	{"ATXP", fATXP, {NULL, NULL}},
#endif
#endif

#elif ATCMD_VER == ATVER_2	// uart at command
	#if CONFIG_WLAN
	{"ATPA", fATPA,},	// set AP
	{"ATPN", fATPN,},	// connect to Network
	{"ATPH", fATPH,},	// set DHCP mode
	{"ATPE", fATPE,},	// set static IP for STA
	{"ATPF", fATPF,},	// set DHCP rule for AP
	{"ATPG", fATPG,},	// set auto connect
	{"ATPM", fATPM,},	// set MAC address
	{"ATPW", fATPW,},	// set Wifi mode
	{"ATWD", fATWD,},
	{"ATWS", fATWS,},
	{"ATW?", fATWx,},

	#if (CONFIG_INCLUDE_SIMPLE_CONFIG)
	{"ATWQ", fATWQ,},
	#endif// #if (CONFIG_INCLUDE_SIMPLE_CONFIG)

	#endif// #if CONFIG_WLAN

#endif//end of #if ATCMD_VER == ATVER_2

	{"AT+CWMODE", fATCWMODE,},
	{"AT+CWDHCP", fATCWDHCP,},
	{"AT+CWLAPOPT", fATCWLAPOPT},
	{"AT+CWLAP",  fATCWLAP},
	{"AT+CWQAP",  fATCWQAP},
	{"AT+CWJAP",  fATCWJAP},
	{"AT+CWSAP",  fATCWSAP},
	{"AT+CIPMUX", fATCIPMUX,},
	{"AT+CIPSTA", fATCIPSTA},
	{"AT+CIPSTAMAC", fATCIPSTAMAC},
	{"AT+CIPAP" , fATCIPAP,},
	{"AT+CIPAPMAC", fATCIPAPMAC,},
	{"AT+CIPDINFO", fATCIPDINFO,},
	{"AT+CIPSTATUS",fATCIPSTATUS,},
};

#if ATCMD_VER == ATVER_2
void print_wifi_at(void *arg)
{
	int index;
	int cmd_len = 0;

	(void) arg;

	cmd_len = sizeof(at_wifi_items) / sizeof(at_wifi_items[0]);
	for (index = 0; index < cmd_len; index++)
		at_printf("\r\n%s", at_wifi_items[index].log_cmd);
}
#endif

void at_wifi_init(void)
{
#if CONFIG_WLAN
	init_wifi_struct();
#endif
	log_service_add_table(at_wifi_items, sizeof(at_wifi_items) / sizeof(at_wifi_items[0]));
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_wifi_init);
#endif
