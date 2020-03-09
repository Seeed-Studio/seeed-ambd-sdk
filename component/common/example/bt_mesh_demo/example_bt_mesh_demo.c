#include "platform_opts.h"
#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include <wifi_conf.h>
#include <osdep_service.h>
#if defined(CONFIG_PLATFORM_8710C)
#include <platform_opts_bt.h>
#endif
#include "bt_config_wifi.h"

#include <gap_conn_le.h> 

#if CONFIG_EXAMPLE_BT_MESH_DEMO

//media
extern void amebacam_broadcast_demo_thread();
//httpd
extern void httpd_demo_init_thread();

void bt_mesh_example_demo_init_thread(void *param)
{
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER) && CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER
	/*Init BT config*/
	bt_config_app_init();

	/*Wait BT config complete*/
	while(get_bt_config_state() != BC_DEV_DISABLED) {
		vTaskDelay(500 / portTICK_RATE_MS);
	}
#endif

#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE) && CONFIG_EXAMPLE_BT_MESH_DEMO_DEVICE
	/*Init BT mesh device*/
	ble_app_main();

	bt_coex_init();

	/*Wait BT init complete*/
	do {
		vTaskDelay(100 / portTICK_RATE_MS);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	} while (new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();
#endif

	vTaskDelete(NULL);
}

void example_bt_mesh_demo(void)
{
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER) && CONFIG_EXAMPLE_BT_MESH_DEMO_PROVISIONER
	//init httpd
	if(xTaskCreate(httpd_demo_init_thread, ((const char*)"httpd_demo_init_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
			printf("\n\r%s xTaskCreate(httpd_demo_init_thread) failed", __FUNCTION__);
	//init amebacam_broadcast
	if(xTaskCreate(amebacam_broadcast_demo_thread, ((const char*)"amebacam_broadcast_demo_thread"), 256, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
			printf("\n\r%s xTaskCreate(amebacam_broadcast_demo_thread) failed", __FUNCTION__);
#endif
	//init bt config/bt mesh
	if(xTaskCreate(bt_mesh_example_demo_init_thread, ((const char*)"bt_mesh_example_demo_init_thread"), 1024, NULL, tskIDLE_PRIORITY + 5 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(bt_mesh_example_demo_init_thread) failed", __FUNCTION__);
}

#endif /* CONFIG_EXAMPLE_BT_MESH_DEMO */

