#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_USBH_VENDOR) && CONFIG_EXAMPLE_USBH_VENDOR
#include <platform/platform_stdlib.h>
#include "osdep_service.h"
#include "usb.h"
#include "usbh_vendor_if.h"

void example_usbh_vendor_thread(void *param)
{
    int status;

    UNUSED(param);
    
    status = usb_init();
    if (status != USB_INIT_OK) {
        printf("\nFail to init USB host controller: %d\n", status);
        goto error_exit;
    }

    status = usbh_vendor_init();
    if (status < 0) {
        printf("\nFail to init USB host vendor driver: %d\n", status);
        usb_deinit();
    }
    
error_exit:
    rtw_thread_exit();
}


void example_usbh_vendor(void)
{
    int status;
    struct task_struct task;
    
    printf("\nUSB host vendor demo started...\n");
    status = rtw_create_task(&task, "example_usbh_vendor_thread", 512, tskIDLE_PRIORITY + 2, example_usbh_vendor_thread, NULL);
    if (status != pdPASS) {
        printf("\nFail to create USB host vendor thread: %d\n", status);
    }
}

#endif
