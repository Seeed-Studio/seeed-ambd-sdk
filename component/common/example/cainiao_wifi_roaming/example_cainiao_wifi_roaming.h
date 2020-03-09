#ifndef __EXAMPLE_WIFI_ROAMING_H__
#define __EXAMPLE_WIFI_ROAMING_H__

//fast reconnect callback fun
extern wlan_init_done_ptr p_wlan_init_done_callback;
extern write_reconnect_ptr p_write_reconnect_ptr;

void example_cainiao_wifi_roaming(void);
void example_cainiao_wifi_roaming_init(void);
int cainio_wifi_roaming_write_ap_to_flash(u8 * data, u32 len);
int cainiao_wlan_init_done_callback(void);
#endif