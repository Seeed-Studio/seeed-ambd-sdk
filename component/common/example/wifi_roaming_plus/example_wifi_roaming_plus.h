#ifndef __EXAMPLE_WIFI_ROAMING_H__
#define __EXAMPLE_WIFI_ROAMING_H__

//fast reconnect callback fun
extern wlan_init_done_ptr p_wlan_init_done_callback;
extern write_reconnect_ptr p_write_reconnect_ptr;

void example_wifi_roaming_plus(void);
void example_wifi_roaming_plus_init(void);
#endif