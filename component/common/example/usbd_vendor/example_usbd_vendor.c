#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_USBD_VENDOR) && CONFIG_EXAMPLE_USBD_VENDOR

#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "basic_types.h"

#include "usb.h"
#include "usbd_vendor_if.h"

void example_usbd_vendor_thread(void *param)
{
    int status = 0;
    
    UNUSED(param);
    
    status = usb_init(USB_SPEED_HIGH);
    if (status != 0) {
        printf("\nFail to init USB device controller: %d\n", status);
        goto example_usbd_vendor_thread_fail;
    }

    status = usbd_vendor_init();
    if (status != 0) {
        printf("\nFail to init USB vendor class: %d\n", status);
        usb_deinit();
        goto example_usbd_vendor_thread_fail;
    }

    rtw_mdelay_os(10000);
    
example_usbd_vendor_thread_fail:
    rtw_thread_exit();
}


void example_usbd_vendor(void)
{
    int status;
    struct task_struct task;
    
    printf("\nUSB device vendor demo started...\n");
    status = rtw_create_task(&task, "example_usbd_vendor_thread", 1024, tskIDLE_PRIORITY + 5, example_usbd_vendor_thread, NULL);
    if (status != pdPASS) {
        printf("\nFail to create USB vendor device thread: %d\n", status);
    }
}

#endif
