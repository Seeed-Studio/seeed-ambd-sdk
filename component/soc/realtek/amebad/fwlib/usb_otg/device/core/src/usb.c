#include "ameba_otg.h"
#include "dwc_os.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_common.h"
#include "usb_gadget.h"
#include "usb_composite.h"
#include "usb.h"

extern void charger_api_set_adapter_current(uint16_t adapter_current);
extern uint8_t g_otg_is_reset_flow_done; // for illegal host
extern int dwc_otg_driver_probe(void);
extern int dwc_otg_driver_stop(void);
extern int dwc_otg_driver_init(void);
extern void dwc_otg_driver_deinit(void);
extern void dwc_otg_driver_remove(void);
extern int usb_composite_init(void);
extern void usb_composite_deinit(void);

usb_info_t usb_info;
u16 g_otg_debug_selection = DBG_USB_COMPOSITE;

xTaskHandle tid_otg_init;

static usb_cfg_t usb_cfg = {
    .bSpeed           = USB_SPEED_HIGH,
        
    .bDeviceClass     = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass  = 0,
    .bDeviceProtocol  = 0,
    .idVendor         = REALTEK_USB_VID,
    .idProduct        = REALTEK_USB_PID,
    .bcdDevice        = 0x0100,
    
    .bmAttributes     = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower        = 50,

    .bIrqTaskPriority = tskIDLE_PRIORITY + 3,
    
    .sManufacturer    = "Realtek",
    .sProduct         = "USB Device",
    .sSerialNumber    = "0123456789AB",
};

void usb_set_cfg(usb_cfg_t *cfg)
{
    if (cfg != NULL) {
        memcpy((void *)&usb_cfg, (void *)cfg, sizeof(usb_cfg_t));
    }
}

usb_cfg_t *usb_get_cfg(void)
{
    return &usb_cfg;
}

static void usb_start(void)
{
    g_otg_is_reset_flow_done = 0;
    usb_info.usb_power_state = USB_ACTIVE;
    usb_info.usb_init_status = USB_INIT_OK;
}

/******************************************************************************
 * Stop USB this stops the LowLevel Part and deregisters USB devices.
 */

static void usb_stop(void)
{
    if (usb_info.usb_power_state >= USB_ATTACHED) {
        usb_composite_stop();
        dwc_otg_driver_stop();
    }

    usb_info.usb_power_state = USB_PDN;
    g_otg_is_reset_flow_done = 0;
}

static void usb_init_thread(void *param)
{
    int result = -1;

    UNUSED(param);
    
    PAD_PullCtrl(_PA_25, GPIO_PuPd_NOPULL);
    PAD_PullCtrl(_PA_26, GPIO_PuPd_NOPULL);
    PAD_PullCtrl(_PA_28, GPIO_PuPd_NOPULL);
    
    result = dwc_otg_driver_init();
    if (result != 0) {
        USB_PRINT_ERROR0("USB low level init fail");
        dwc_otg_driver_deinit();
        return;
    }

    result = dwc_otg_driver_probe();
    if (result != 0) {
        USB_PRINT_ERROR0("USB low level start fail");
        dwc_otg_driver_stop();
        dwc_otg_driver_deinit();
        return ;
    }

    result = usb_composite_init();
    if (result != 0) {
        USB_PRINT_ERROR0("USB composite init fail");
        usb_composite_deinit();
        dwc_otg_driver_stop();
        dwc_otg_driver_deinit();
        return ;
    }

    usb_start();
    
    rtw_mdelay_os(1);
    vTaskDelete(NULL);
}

static int wait_usb_ready(void){
	int retry = 100;
	while(usb_info.usb_init_status == USB_INIT_NONE){
		rtw_mdelay_os(100);
		if((--retry)==0) {
			break;
        }
	};
	return usb_info.usb_init_status;
}

int usb_init(void)
{
    int ret = 0;
    usb_info.usb_status = USB_STATUS_INIT;
    usb_info.usb_power_state = USB_PDN;
    usb_info.usb_init_status = USB_INIT_NONE;
    
    if (pdTRUE != xTaskCreate(usb_init_thread, "usb_init_thread", 1024,
            NULL, tskIDLE_PRIORITY + 5, &tid_otg_init)) {
        USB_PRINT_ERROR0("Create USB init thread fail");
        ret = 1;
    }

    if (ret == 0) {
        ret = wait_usb_ready();
    }

    return ret;
}

int usb_deinit(void)
{
    int ret = 0;
    
    usb_stop();
    usb_composite_deinit();
    dwc_otg_driver_deinit();
    
    usb_info.usb_status = USB_STATUS_INIT;
    usb_info.usb_init_status = USB_INIT_NONE;

    return ret;
}

int usb_get_status(void)
{
	return usb_info.usb_status;
}
