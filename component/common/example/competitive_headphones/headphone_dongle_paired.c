#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>
#include "wifi_structures.h"
#include "i2s_api.h"
#include "freertos_service.h"


#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES

int headphone_pair()
{
	static rtw_network_info_t wifi = {0};
	static unsigned char password[65]={0};
	int mode, ret;
	unsigned long tick1 = xTaskGetTickCount();
	unsigned long tick2, tick3;

	int timeout = 20;
	while(timeout > 0) {
		if(rltk_wlan_running(0)) {
			break;
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
		timeout --;
	}
	
	strcpy((char *)wifi.ssid.val, "headphone_dongle");
	wifi.ssid.len = strlen("headphone_dongle");
	wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
	strcpy((char *)password, "12345678");
	wifi.password = password;
	wifi.password_len = strlen("12345678");

	printf("\n\rJoining BSS by SSID %s...\n\r", (char*)wifi.ssid.val);
	ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
					wifi.password_len, wifi.key_id, NULL);

	if(ret!= RTW_SUCCESS){
		if(ret == RTW_INVALID_KEY)
			printf("\n\rERROR:Invalid Key ");
		
		printf("\n\rERROR: Can't connect to AP");
		return -1;
	}
	tick2 = xTaskGetTickCount();
	printf("\r\nConnected after %dms.\n", (tick2-tick1));
#if CONFIG_LWIP_LAYER
		/* Start DHCPClient */
	if(LwIP_DHCP(0, DHCP_START)!= DHCP_ADDRESS_ASSIGNED)
		return -1;
	tick3 = xTaskGetTickCount();
	printf("\r\n\nGot IP after %dms.\n", (tick3-tick1));
#endif
	printf("\n\r");
	return 0;

	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS){ 
		printf("\r\n\r\n\r\n>>>>>>>>>>>>>>Wifi is disconnected!!Please connect!!<<<<<<<<<<<<<<<<<\r\n\r\n\r\n");
		vTaskDelay(10000);
	}

}

#endif

