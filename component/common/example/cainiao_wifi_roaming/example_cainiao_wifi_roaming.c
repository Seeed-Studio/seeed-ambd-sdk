#include <autoconf.h>
#include <FreeRTOS.h>
#include "task.h"
#include <platform/platform_stdlib.h>
#include <wifi/wifi_conf.h>
#include <lwip_netconf.h>
#include "netif.h"
#include "wlan_fast_connect/example_wlan_fast_connect.h"
#include "flash_api.h"
#include "device_lock.h"
#include "cainiao_wifi_roaming/example_cainiao_wifi_roaming.h"

#include "netif/etharp.h"
#include "lwip/opt.h"

#include "lwip/ip_addr.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"

/****************************
 *                Macro                    *  
 ****************************/

#define RSSI_ROAMING_THRESHOLD -65	//when current ap rssi < RSSI_ROAMING_THRESHOLD, start connect to an other better ap.
#define RSSI_SCAN_THRESHOLD -60	//when current ap rssi < RSSI_SCAN_THRESHOLD, start to scan a better ap.
#define CAINIAO_MAX_CH_NUM 4
#define SCAN_BUFLEN 500 	//each scan list length= 14 + ssid_length(32MAX). so SCAN_BUFLEN should be AP_NUM*(14+32) at least

#define  BETTER_RSSI_ON 10
#define MAX_AP_NUM 16

#define CAINIAO_ROAMING_DBG 0 		//for debug log
#if	CAINIAO_ROAMING_DBG
#define ROAMING_DBG	printf
#else
#define ROAMING_DBG
#endif
#define CAINIAO_SCAN_DBG 0 		//for scan debug log
#if	CAINIAO_SCAN_DBG
#define SCAN_DBG	printf
#else
#define SCAN_DBG
#endif
#define CAINIAO_TIMER_DBG 1 		//for timer debug log
#if	CAINIAO_TIMER_DBG
#define TIMER_DBG	printf
#else
#define TIMER_DBG
#endif
#define CAINIAO_DHCP_DBG 0 		//for DHCP debug log
#if	CAINIAO_DHCP_DBG
#define DHCP_DBG	printf
#else
#define DHCP_DBG
#endif
#define CAINIAO_DBG 1 		//for useful debug log
#if	CAINIAO_DBG
#define USEFUL_DBG	printf
#else
#define USEFUL_DBG
#endif

#ifndef WLAN0_NAME
#define WLAN0_NAME		"wlan0"
#endif
#ifndef WLAN1_NAME
#define WLAN1_NAME		"wlan1"
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 			6
#endif

typedef struct wifi_roaming_ap
{
	u8 	ssid[33];
	u8 	bssid[ETH_ALEN];
	u8	channel;
	rtw_security_t		security_type;
	u8 	password[65];
	u8	key_idx;
	s32	rssi;		
#if CONFIG_LWIP_LAYER
	u8	ip[4];
#endif
}wifi_roaming_ap_t;

struct cainiao_dhcp_info{
	u8 dhcp_bssid[ETH_ALEN];
	uint32_t dhcp_ip;
}cainiao_dhcp_info_t;

struct cainiao_roaming_data {
	u8 FAT_AP;
	u8 num;
	u8 ap_n;
	//u8 dhcp_n;
	struct wlan_fast_reconnect ap_info;
	u32 channel[CAINIAO_MAX_CH_NUM];	
	struct cainiao_dhcp_info dhcp_info[MAX_AP_NUM];	
}cainiao_roaming_data_t;

typedef struct channel_plan_2g
{
	u8	channel[14];
	u8	len;
}channel_plan_2g_t;

channel_plan_2g_t channel_plan_2g = {{1,2,3,4,5,6,7,8,9,10,11,12,13}, 13};

enum {
	FAST_CONNECT_SPECIFIC_CH = 0,
	FAST_CONNECT_ALL_CH  = 1
};
typedef u8 fast_connect_ch_t;

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM]; 
#endif

//static wifi_roaming_ap_t *ap_list;
static wifi_roaming_ap_t ap_list = {0};

static u8 pscan_enable = _TRUE; // if set _TRUE, please set pscan_channel_list
static u8 pscan_channel_list[]={1,6,11};// set by customer
static u8 FAT_AP = 0xff ;
static uint32_t find_bssid = 0;

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
extern uint32_t offer_ip;
//extern u8 is_the_same_ap;
#endif
/*****************************************
 *              functions     defination                              *
 ****************************************/
/* set tx pause but enable management frames to tx . give api later*/
int wifi_set_tx_pause_enable(void)
{
	HAL_WRITE8(0x40080000, 0x522, 0xef);
	return 0;
}
int wifi_set_tx_pause_disable(void)
{
	HAL_WRITE8(0x40080000, 0x522, 0x0);
	return 0;
}

int  LwIP_arp_update_gw_ethaddr(struct netif* pnetif)
{
	err_t result;

	etharp_cleanup_netif(pnetif);
	
	 /* create an ARP query for the offered IP address, expecting that no host
         responds, as the IP address should not be in use. */
         result = etharp_query(pnetif, &(pnetif->gw), NULL);
         if (result != ERR_OK) {
                   LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("arp_request : could not perform ARP query\n"));
         }

	return 0;
}


//extern void time_mers_set(void);
u32 cainiao_wifi_roaming_find_ap_from_scan_buf(char*buf, int buflen, char *target_ssid, void *user_data);

int cainio_wifi_roaming_write_ap_to_flash(u8 * data, u32 len)
{
	unsigned long tick1 = xTaskGetTickCount();
	unsigned long tick2, tick3, tick4, tick5;
	flash_t flash;
	u8 i=0;
	struct cainiao_roaming_data read_data = {0};
	u8 ap_change = 0;

	if(!data)
            return -1;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (u8 *) &read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	// if connect from ATWC, when there is no data in flash: 4+ap_info
	if(read_data.num == 0xff){
		read_data.ap_n = 0;
		read_data.num = 0;
		//read_data.FAT_AP=1;
	}// check FAT_AP state
	if((FAT_AP == FALSE) && (read_data.FAT_AP == 0xff)){ // if env changed, please erase the flash first
		read_data.FAT_AP = FAT_AP;
	}

	//store the dhcp_info into flash after dhcp done if FAT AP
	if(len == sizeof(struct cainiao_dhcp_info)){
	ROAMING_DBG("\r\n %s():add new AP  bssid & offer_IP to flash", __func__);
	USEFUL_DBG("****2. flash write dhcp_info");
		if (read_data.FAT_AP == FALSE) {
			read_data.ap_n =1;
		} else {
			int n = read_data.ap_n;
			//if(ap_list.bssid[0]||ap_list.bssid[1]||ap_list.bssid[2]||ap_list.bssid[3]||ap_list.bssid[4]||ap_list.bssid[5]){ //ap_list.bssid!=0
			for(n = read_data.ap_n; n>=0; n--){
				if(memcmp(read_data.dhcp_info[n].dhcp_bssid,((struct cainiao_dhcp_info *)data)->dhcp_bssid, ETH_ALEN)==0){
					read_data.dhcp_info[n].dhcp_ip = ((struct cainiao_dhcp_info*)data)->dhcp_ip;
					ROAMING_DBG("%s(): have stored this bssid\n",__func__);
					break;					
				}
				if(n ==0){
					read_data.ap_n ++ ;
					memcpy((u8 *)(read_data.dhcp_info[read_data.ap_n -1].dhcp_bssid), ((struct cainiao_dhcp_info *)data)->dhcp_bssid, ETH_ALEN);
					read_data.dhcp_info[read_data.ap_n -1].dhcp_ip = ((struct cainiao_dhcp_info *)data)->dhcp_ip;
					ROAMING_DBG("%s(): add ap_bssid & sta_ip index(%d)",__func__, (read_data.ap_n -1) );
					}
				}
			}
	device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, FAST_RECONNECT_DATA);	
		flash_stream_write(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (uint8_t *)  &read_data);
		//ROAMING_DBG("dhcp_info = "MAC_FMT", dhcp_ip = %d", MAC_ARG(read_data.dhcp_info[0].dhcp_bssid), read_data.dhcp_info[0].dhcp_ip);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	tick2 = xTaskGetTickCount();
	TIMER_DBG(" %s(): store the dhcp info into flash in %ms\n",__func__, (tick2 - tick1));
	return 0;
	}


	//store scan channel after DHCP get IP. write after DHCP get ip; store AP_info if flash is null
	if(read_data.num < CAINIAO_MAX_CH_NUM) {
		USEFUL_DBG("****1. flash write wlan_fast_reconnect info");
		if(read_data.num){
			/*check if ap info {ssid/password/security_type} has changed*/
			if(memcmp((u8 *)((struct wlan_fast_reconnect *)data)->psk_essid, (u8 *) (read_data.ap_info.psk_essid), 32)) {
				printf("ap ssid change\n");
				ap_change = 1;
			} else if(memcmp((u8 *)((struct wlan_fast_reconnect *)data)->psk_passphrase, (u8 *) (read_data.ap_info.psk_passphrase), 32)) {
				printf("ap password change\n");
				ap_change = 1;
			} else if(((struct wlan_fast_reconnect *)data)->security_type != read_data.ap_info.security_type) {
				printf("ap security type change\n");
				ap_change = 1;
			} else if((FAT_AP!=0xff) && (FAT_AP != read_data.FAT_AP)) {
				printf("FAT or FIT ap structure change\n");
				ap_change = 1;
			}else { /*ap info doesn't change*/
				for(i = 0;i < read_data.num; i++){
					if(read_data.channel[i] == ((struct wlan_fast_reconnect *)data)->channel) {
						ROAMING_DBG("Already stored this channel(%d)\n",((struct wlan_fast_reconnect *)data)->channel);
						 return -1;
					}		
				}
			}		
		}
		if(ap_change) {
			printf("erase flash and restore new ap info\n");
			memset((u8 *)&read_data,0xff,sizeof(struct  cainiao_roaming_data));
			read_data.num = 1;
		}else {
			printf("Add a new ap channel  into flash\n");
			read_data.num++;
		}
		read_data.channel[read_data.num-1] = ((struct wlan_fast_reconnect *)data)->channel;//store channel
		if(read_data.num == 1){
			memcpy((u8 *)&read_data.ap_info, data,sizeof(struct wlan_fast_reconnect));//store fast connect info
			}
		device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_erase_sector(&flash, FAST_RECONNECT_DATA);	
			flash_stream_write(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (uint8_t *)  &read_data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		tick3 = xTaskGetTickCount();
		TIMER_DBG("%s(): flash write channel total time : %d ms\n\n",__func__, (tick3 - tick1));	
		} else {
		ROAMING_DBG("%s():flash has %d channels, no need to add more\n", __func__,read_data.num);
	}
	return 0;
}

int cainiao_wlan_fast_connect(struct cainiao_roaming_data *data, u8 scan_type)
{
	unsigned long tick1 = xTaskGetTickCount();
	unsigned long tick2, tick3, tick4, tick5, tick6;

	uint32_t	channel;
	uint32_t    security_type;
	uint8_t     pscan_config;
	char key_id[2] = {0};
	int ret;
	uint32_t wifi_retry_connect = 3;//For fast wifi connect retry
	rtw_network_info_t wifi = {0};
	int n = data->ap_n;
	struct cainiao_dhcp_info copy_dhcp_flash[n];
	u8 ap_ethaddr[ETH_ALEN];

#if CONFIG_LWIP_LAYER
	netif_set_up(&xnetif[0]);
#endif

#if CONFIG_AUTO_RECONNECT
	wifi_set_autoreconnect(0);
#endif

	//time_mers_set();
	ROAMING_DBG("\r\n %s()\n", __func__);
	memcpy(psk_essid, data->ap_info.psk_essid, sizeof(data->ap_info.psk_essid));
	memcpy(psk_passphrase, data->ap_info.psk_passphrase, sizeof(data->ap_info.psk_passphrase));
	memcpy(wpa_global_PSK, data->ap_info.wpa_global_PSK, sizeof(data->ap_info.wpa_global_PSK));

	channel = data->ap_info.channel;
		sprintf(key_id,"%d",(char) (channel>>28));
		channel &= 0xff;
		security_type = data->ap_info.security_type;
		pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
		//set partial scan for entering to listen beacon quickly
WIFI_RETRY_LOOP:
		if(scan_type == FAST_CONNECT_SPECIFIC_CH) {
			ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
			if(ret < 0){
				free(data);
				return -1;
			}
		}
		wifi.security_type = security_type;
		//SSID
		strcpy((char *)wifi.ssid.val, (char*)psk_essid);
		wifi.ssid.len = strlen((char*)psk_essid);

		switch(security_type){
			case RTW_SECURITY_WEP_PSK:
				wifi.password = (unsigned char*) psk_passphrase;
				wifi.password_len = strlen((char*)psk_passphrase);
				wifi.key_id = atoi((const char *)key_id);
				break;
			case RTW_SECURITY_WPA_TKIP_PSK:
			case RTW_SECURITY_WPA2_AES_PSK:
#ifdef CONFIG_SAE_SUPPORT
			case RTW_SECURITY_WPA3_AES_PSK:
#endif
				wifi.password = (unsigned char*) psk_passphrase;
				wifi.password_len = strlen((char*)psk_passphrase);
				break;
			default:
				break;
		}

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
	if(data->FAT_AP == TRUE) { //DHCP reboot
		USEFUL_DBG("****1. c_connect to "MAC_FMT"\n", MAC_ARG(ap_list.bssid));
		memcpy(copy_dhcp_flash, &(data->dhcp_info), n * sizeof(struct cainiao_dhcp_info));
		int i = 0;
		//while( (i <= n) && (!memcmp(ap_list.bssid, 0x0, ETH_ALEN))) {
		for(i = 0; i<= n; i++){
			//DHCP_DBG("check (%d)ap_mac:"MAC_FMT", ap_list:"MAC_FMT"\n", i, MAC_ARG(copy_dhcp_flash[i].dhcp_bssid), MAC_ARG(ap_list.bssid));
			if(memcmp(ap_list.bssid, copy_dhcp_flash[i].dhcp_bssid, ETH_ALEN) == 0){
				data->ap_info.offer_ip = copy_dhcp_flash[i].dhcp_ip;
				find_bssid = i+1;
				ROAMING_DBG("\n\r Find the ehter_addr in flash(%d) \n",find_bssid);
				break;
			}
			if(i == n){
			DHCP_DBG("%s(): NO ap_ethaddr found in flash !!\n",__func__);
			}
			i++;
		}	
	}else {  //DHCP discover
		DHCP_DBG("At fit_AP or connecte to a new  AP\n");
	}
	offer_ip = data->ap_info.offer_ip;
	//DHCP_DBG("offer_IP : %x\n", offer_ip);
#endif
		u8 before_mac[ETH_ALEN];
		uint32_t before_ip0 = xnetif[0].ip_addr.addr;
		wifi_get_ap_bssid(before_mac);
		ROAMING_DBG("get ap bssid before "MAC_FMT"\r", MAC_ARG(before_mac));
		DHCP_DBG("\r%s():cainiao beforeIP= %x \n",__func__, before_ip0);	

		//pause tx while wifi connecting..
		wifi_set_tx_pause_enable();
		ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
			wifi.password_len, wifi.key_id, NULL);
		wifi_set_tx_pause_disable();

		if(ret != RTW_SUCCESS){
                  wifi_retry_connect--;
                  if(wifi_retry_connect > 0){
                      printf("wifi retry\r\n");
                      goto WIFI_RETRY_LOOP;
                  }
		}
		if(ret == RTW_SUCCESS) {
			tick2 = xTaskGetTickCount();
			TIMER_DBG("\n 1. cainiao_connect after %d s\n",(tick2 -tick1));

			// 1. do fast DHCP in FAT APs
			//ROAMING_DBG("fat=%d ; flash = %d\n",FAT_AP, data->FAT_AP);
			if(data->FAT_AP == FALSE) {
				ROAMING_DBG("\r\n %s():  this is an FIT_AP, no DHCP!\n", __func__);
			}else {
				LwIP_DHCP(0, DHCP_START);
				#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
				LwIP_arp_update_gw_ethaddr(&xnetif[0]);// to solve  gw arp_list update @roaming fast DHCP
				#endif

				u8 after_mac[ETH_ALEN];
				uint32_t after_ip0 = xnetif[0].ip_addr.addr;
				wifi_get_ap_bssid(after_mac);
				ROAMING_DBG("get ap bssid "MAC_FMT"\r", MAC_ARG(after_mac));
				printf("\r%s():cainiao IPaddress= %x, offer_ip = %x\n",__func__, after_ip0,offer_ip);
				USEFUL_DBG("****2. c_connectED to "MAC_FMT", get ip =%x\n", MAC_ARG(after_mac),after_ip0);
				if((offer_ip == after_ip0)){
					FAT_AP = FALSE;
				}
				// 3. write the AP messages into flash. not FIT_ap; get_IP != offer_IP
				struct cainiao_dhcp_info store_dhcp_info = {0};
				memcpy(store_dhcp_info.dhcp_bssid, after_mac, ETH_ALEN);
				store_dhcp_info.dhcp_ip = after_ip0;
				cainio_wifi_roaming_write_ap_to_flash((u8 *)&store_dhcp_info, sizeof(struct cainiao_dhcp_info));
			}
			tick3 = xTaskGetTickCount();
			TIMER_DBG("cainiao DHCP time: %d ms,   cainiao_get IP after %d ms\n",(tick3 -tick2), (tick3 -tick1));
		}


			// 2. (only once) check if FAT AP for the first connection. no data in flash & reboot
#if 0
			if((data->FAT_AP == 0xff) && (FAT_AP == 0xff)){
				wifi_get_ap_bssid(ap_ethaddr);
				ROAMING_DBG("get ap bssid "MAC_FMT"\r", MAC_ARG(ap_ethaddr));
				 struct eth_addr *arp_eth_ret = NULL;
				 
				int diff = LwIP_FatAP_check(&xnetif[0], &(xnetif[0].gw), ap_ethaddr);
				ROAMING_DBG("%x:%x:%x:%x:%x:%x [%d]\n",ap_ethaddr[0], ap_ethaddr[1], ap_ethaddr[2], ap_ethaddr[3],ap_ethaddr[4],ap_ethaddr[5],diff);
				if(!diff) {
					printf("\r *** FAT_AP structure***\n");
					FAT_AP =1;
				}else {
					FAT_AP =0;
					printf("\r *** FIT_AP structure***\n");
				}
			}
#endif


					
#if CONFIG_AUTO_RECONNECT
		wifi_set_autoreconnect(2);
#endif
tick4 = xTaskGetTickCount();
TIMER_DBG("\ncainiao_fast_connect done  after %d ms\n",(tick4 -tick1));
USEFUL_DBG("****3. c_connection time =%d\n", (tick4 - tick1));
	return ret;
}
	
int cainiao_wlan_init_done_callback(void)
{
	flash_t		flash;
	struct cainiao_roaming_data read_data = {0};

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (u8 *) &read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	/* Check whether stored flash profile is empty */
	if((read_data.num == 0xff) || (read_data.num > CAINIAO_MAX_CH_NUM))
		printf("Fast connect profile is empty, abort fast connection\n");
	/* Find the best ap in flash profile */
	else {
		ROAMING_DBG("[Cainiao Fast connect] Connect to the best ap\n");
		cainiao_wlan_fast_connect(&read_data,FAST_CONNECT_ALL_CH);
	}
	return 0;
}
	
u32 cainiao_wifi_roaming_find_ap_from_scan_buf(char*buf, int buflen, char *target_ssid, void *user_data)
{
	u32 target_security = *(u32 *)user_data;
	
	int plen = 0;	
	while(plen < buflen)
	{
		u32 len, ssid_len, security_mode, security_type, channel;
		s32 rssi;
		u8 *mac, *ssid;				
		// len
		len = (int)*(buf + plen);
		// check end
		if(len == 0|| len == strlen(target_ssid)) break;
		// if len == ssid_len, it means driver dont do scan,maybe it is busy now, buf detail is the same as it initialized
		// mac
		mac =(u8*)(buf + plen + 1);
		// rssi
		rssi = *(s32*)(buf + plen + 1 + 6);
		// security_mode offset = 11
		security_mode = (u8)*(buf + plen + 1 + 6 + 4);
		switch(security_mode){
			case IW_ENCODE_ALG_NONE:
				security_type = RTW_SECURITY_OPEN;
				break;
			case IW_ENCODE_ALG_WEP:
				security_type = RTW_SECURITY_WEP_PSK;
				break;
			case IW_ENCODE_ALG_TKIP:
				security_type = RTW_SECURITY_WPA_TKIP_PSK;
				break;
			case IW_ENCODE_ALG_CCMP:
				security_type = RTW_SECURITY_WPA2_AES_PSK;
				break;
			default:
				break;
		}
		// channel
		channel = *(buf + plen + 1 + 6 + 4 + 1 + 1);
		// ssid
		ssid_len = len - 1 - 6 - 4 - 1 - 1 - 1;
		ssid = (u8*)(buf + plen + 1 + 6 + 4 + 1 + 1 + 1);
		ROAMING_DBG("Scan ap:"MAC_FMT"(%d)\n", MAC_ARG(mac),channel);
		if(target_security == security_type ||
		((target_security & (WPA2_SECURITY|WPA_SECURITY))&&(target_security & (WPA2_SECURITY|WPA_SECURITY)))){
			if(ap_list.rssi < rssi){
				ROAMING_DBG("rssi(%d) is better than last(%d)\n",rssi,ap_list.rssi);
				USEFUL_DBG("****1.Scan ap:"MAC_FMT"(%d)\rrssi(%d) is better than last(%d)\n ", MAC_ARG(mac),channel,rssi,ap_list.rssi);
				memset(&ap_list, 0 , sizeof(wifi_roaming_ap_t));
				memcpy(ap_list.bssid, mac, ETH_ALEN);
				ap_list.channel = channel;
				ap_list.rssi = rssi;
			}
		}
		plen += len;			
	}
	return 0;
}

int cainiao_wifi_roaming_scan(struct cainiao_roaming_data  read_data, s32 cur_rssi)
{
	wifi_roaming_ap_t	roaming_ap;
	rtw_wifi_setting_t	setting;
	u8	pscan_config;
	channel_plan_2g_t channel_plan_temp_2g = channel_plan_2g;
	u8 ch = 0;
	
	memset(&setting, 0, sizeof(rtw_wifi_setting_t));
	memset(&roaming_ap, 0, sizeof(wifi_roaming_ap_t));
	roaming_ap.rssi = -100;

	wifi_get_setting(WLAN0_NAME,&setting);
	strcpy((char*)roaming_ap.ssid, (char const*)setting.ssid);
	roaming_ap.security_type =  setting.security_type;
	strcpy((char*)roaming_ap.password, (char const*)setting.password);
	roaming_ap.key_idx = setting.key_idx;

	if(RTW_ERROR == wifi_get_ap_bssid(roaming_ap.bssid))
		printf("\r\n get AP BSSID FAIL!");

	if(pscan_enable == _TRUE){
		/*scan specific channels*/
		if(0 < read_data.num && read_data.num < CAINIAO_MAX_CH_NUM){
			ROAMING_DBG("\r\n %s():try to find a better ap in flash\n", __func__);
			while(read_data.num) {
				//time_mers_set();
				pscan_channel_list[0]=read_data.channel[read_data.num - 1];
				read_data.num--;
				pscan_config = PSCAN_ENABLE;
				wifi_set_pscan_chan(pscan_channel_list, &pscan_config, 1);
			unsigned long tick1 = xTaskGetTickCount();
				//netif_set_down(&xnetif[0]);
				wifi_set_tx_pause_enable();
				wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))cainiao_wifi_roaming_find_ap_from_scan_buf, 
					(void *)&roaming_ap.security_type, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
				wifi_set_tx_pause_disable();
				//netif_set_up(&xnetif[0]);
			unsigned long tick2 = xTaskGetTickCount();
				SCAN_DBG("scan1 : %d ms\n",(tick2 - tick1));
				channel_plan_temp_2g.channel[pscan_channel_list[0] -1] = 0;
				//time_mers_set();
				vTaskDelay(500);
			}
		}
		if(ap_list.rssi - cur_rssi > BETTER_RSSI_ON) {
			ROAMING_DBG("\r\n %s():Find the best ap in flash successful, rssi = %d\n", __func__,ap_list.rssi);
		} else {
			/*scan other channels*/
			ROAMING_DBG("\r\n %s():Find the best ap in flash fail, rssi = %d, try to find in other channels\n", __func__,ap_list.rssi);
			for(ch = 0;ch < channel_plan_temp_2g.len;ch++) {
				if(channel_plan_temp_2g.channel[ch]) {
					pscan_channel_list[0]=channel_plan_temp_2g.channel[ch];
					pscan_config = PSCAN_ENABLE;				
					wifi_set_pscan_chan(pscan_channel_list, &pscan_config, 1);
				unsigned long tick1 = xTaskGetTickCount();
				//netif_set_down(&xnetif[0]);
				wifi_set_tx_pause_enable();
					wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))cainiao_wifi_roaming_find_ap_from_scan_buf, 
						(void *)&roaming_ap, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
				wifi_set_tx_pause_disable();
				//netif_set_up(&xnetif[0]);
				unsigned long tick2 = xTaskGetTickCount();
				SCAN_DBG("scan 2: %d ms\n",(tick2 - tick1));
					vTaskDelay(500);
				}
			}
		}
	}else {
		wifi_set_tx_pause_enable();
		wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))cainiao_wifi_roaming_find_ap_from_scan_buf, 
			(void *)&roaming_ap, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
		wifi_set_tx_pause_disable();
	}
	if((memcmp(roaming_ap.bssid, ap_list.bssid, ETH_ALEN))&& ((ap_list.rssi  - cur_rssi) > BETTER_RSSI_ON)){
		ROAMING_DBG("\r\n %s():Find a ap\n", __func__);
		USEFUL_DBG("****2.Scan find an ap:"MAC_FMT" rssi(%d)\n ", MAC_ARG(ap_list.bssid), ap_list.rssi);
		return 1;
	} else{
		ROAMING_DBG("\r\n %s():Didn't find anyother ap\n", __func__);
		USEFUL_DBG("****2. scan NO better AP!\n");
		return 0;
	}


}

void cainiao_wifi_roaming_thread(void *param)
{
	printf("\n %s()\n",__func__);
	int	ap_rssi;
	u32	polling_count =0;
	u32 ap_valid = 4;
	struct cainiao_roaming_data read_data = {0};
	flash_t flash;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (u8 *) &read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	while(1){//wait wifi connect
		if(wifi_is_up(RTW_STA_INTERFACE)&&(RTW_SUCCESS == wifi_is_ready_to_transceive(RTW_STA_INTERFACE))){
			break;
		}
		else
			vTaskDelay(1000);
	}
	vTaskDelay(10000);//wait rssi stable

	while(1)	{
		if(wifi_is_up(RTW_STA_INTERFACE)&&(RTW_SUCCESS == wifi_is_ready_to_transceive(RTW_STA_INTERFACE))) {		
			wifi_get_rssi(&ap_rssi);
			ROAMING_DBG("\r\n %s():Check if need to scan an other ap(rssi:%d)\n", __func__,ap_rssi);
			if((ap_rssi < RSSI_SCAN_THRESHOLD)) {
				if( polling_count >= 1) {
					ROAMING_DBG("\r\n %s():Start scan, current rssi(%d) < scan threshold rssi(%d) \n", __func__,ap_rssi,RSSI_SCAN_THRESHOLD);
					//ap_list = (wifi_roaming_ap_t *)malloc(sizeof(wifi_roaming_ap_t));
					//memset(ap_list, 0, sizeof(wifi_roaming_ap_t));
					ap_list.rssi = -100;
					//device_mutex_lock(RT_DEV_LOCK_FLASH);
					//flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  cainiao_roaming_data), (u8 *) &read_data);
					//device_mutex_unlock(RT_DEV_LOCK_FLASH);
	
					/*find a better ap*/
					if(cainiao_wifi_roaming_scan(read_data,ap_rssi) && ap_list.rssi - ap_rssi > 10) {
						ROAMING_DBG("\r\n %s():Find a better ap, rssi(%d)\n", __func__,ap_list.rssi);
						while(ap_valid){				
							if(wifi_is_up(RTW_STA_INTERFACE)&&(RTW_SUCCESS == wifi_is_ready_to_transceive(RTW_STA_INTERFACE))){
								wifi_get_rssi(&ap_rssi);
								ROAMING_DBG("\r\n %s():Current rssi(%d),check if need to connect an other ap\n", __func__,ap_rssi);
								if( (ap_rssi < RSSI_ROAMING_THRESHOLD)){
									/*connect a better ap*/
									ROAMING_DBG("\r\n %s():Start roaming, current rssi(%d) < roaming threshold rssi(%d),target ap rssi(%d)\n", \
										__func__,ap_rssi,RSSI_ROAMING_THRESHOLD,ap_list.rssi);
									//USEFUL_DBG("****1. roaming to "MAC_FMT" at rssi(%d) > cur_rssi(%d)",MAC_ARG(ap_list.bssid), ap_list.rssi, ap_rssi);
									read_data.ap_info.channel = ap_list.channel;
									cainiao_wlan_fast_connect(&read_data,FAST_CONNECT_SPECIFIC_CH);
									break;
								}
								else if(ap_rssi > RSSI_SCAN_THRESHOLD + 5) {
									/*no need to roaming*/
									ROAMING_DBG("\r\n %s():Current rssi=%d,no need to roaming\n",__func__,ap_rssi);
									break;
								}
								ap_valid--;
							} else 
								break;
							vTaskDelay(1000);
						}
					} else
						ROAMING_DBG("No other better APs!\n ");
					//free(ap_list);
					polling_count = 0;	
				}else				
				polling_count++;
			}else
				polling_count = 0;
		}
		vTaskDelay(1000);
	}
	//vTaskDelete(NULL);
}

void example_cainiao_wifi_roaming(void)
{
	if(xTaskCreate(cainiao_wifi_roaming_thread, ((const char*)"wifi_roaming_thread"), 1024, NULL, tskIDLE_PRIORITY + 1 , NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(wifi_roaming_thread) failed", __FUNCTION__);

	return;
}

void example_cainiao_wifi_roaming_init(void)
{	
	// Call back from wlan driver after wlan init done
	p_wlan_init_done_callback = cainiao_wlan_init_done_callback;

	// Call back from application layer after wifi_connection success
	p_write_reconnect_ptr = cainio_wifi_roaming_write_ap_to_flash;
}
