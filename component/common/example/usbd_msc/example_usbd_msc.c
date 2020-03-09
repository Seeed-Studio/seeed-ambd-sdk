#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_USBD_MSC) && CONFIG_EXAMPLE_USBD_MSC
#include <platform/platform_stdlib.h>
#include "usb.h"
#include "usbd_msc_if.h"
#include "osdep_service.h"

// This configuration is used to enable a thread to check hotplug event
// and reset USB stack to avoid memory leak, only for example.
// For bus-powered device, it shall be set to 0.
// For self-powered device:
// * it is suggested to check the hotplug event via hardware VBUS GPIO interrupt
//   support other than this software polling thread.
// * while if there is no hardware VBUS GPIO interrupt support, set this configuration
//   to 1 for a better support for hotplug.
// * Set this configuration to 0 to save CPU timeslice for better performance, if
//   there is no need for hotplug support.
#define CONFIG_USDB_MSC_CHECK_USB_STATUS   1

#if CONFIG_USDB_MSC_CHECK_USB_STATUS
static void msc_check_usb_status_thread(void *param)
{
    int ret = 0;
    int usb_status = USB_STATUS_INIT;
    static int old_usb_status = USB_STATUS_INIT;

    UNUSED(param);

    for (;;) {
        rtw_mdelay_os(100);
        usb_status = usb_get_status();
        if (old_usb_status != usb_status) {
            old_usb_status = usb_status;
            if (usb_status == USB_STATUS_DETACHED) {
                printf("\n\rUSB DETACHED\n\r");
                usbd_msc_deinit();
                usb_deinit();
                ret = usb_init(USB_SPEED_HIGH);
                if (ret != 0) {
                    printf("\n\rUSB re-init fail\n\r");
                    break;
                }
                ret = usbd_msc_init();
                if (ret != 0) {
                    printf("\n\rUSB MSC re-init fail\n\r");
                    usb_deinit();
                    break;
                }
            } else if (usb_status == USB_STATUS_ATTACHED) {
                printf("\n\rUSB ATTACHED\n\r");
            } else {
                printf("\n\rUSB INIT\n\r");
            }
        }
    }
    
    rtw_thread_exit();
}
#endif // CONFIG_USDB_MSC_CHECK_USB_STATUS

static void example_usbd_msc_thread(void *param)
{
    int status = 0;
#if CONFIG_USDB_MSC_CHECK_USB_STATUS
    struct task_struct task;
#endif
    
    UNUSED(param);
    
    status = usb_init(USB_SPEED_HIGH);
    if (status != 0) {
        printf("\n\rUSB init fail\n\r");
        goto example_usbd_msc_thread_fail;
    }
    
    status = usbd_msc_init();
    if (status != 0) {
        printf("\n\rUSB MSC init fail\n\r");
        usb_deinit();
        goto example_usbd_msc_thread_fail;
    }

#if CONFIG_USDB_MSC_CHECK_USB_STATUS
    status = rtw_create_task(&task, "msc_check_usb_status_thread", 512, tskIDLE_PRIORITY + 2, msc_check_usb_status_thread, NULL);
    if (status != pdPASS) {
        printf("\n\rUSB create status check thread fail\n\r");
        usbd_msc_deinit();
        usb_deinit();
        goto example_usbd_msc_thread_fail;
    }
#endif // CONFIG_USDB_MSC_CHECK_USB_STATUS

    printf("\n\rUSBD MSC demo started\n\r");
    
example_usbd_msc_thread_fail:
    rtw_thread_exit();
}

void example_usbd_msc(void)
{
    int ret;
    struct task_struct task;
    
    ret = rtw_create_task(&task, "example_usbd_msc_thread", 1024, tskIDLE_PRIORITY + 5, example_usbd_msc_thread, NULL);
    if (ret != pdPASS) {
        printf("\n\rUSBD MSC create thread fail\n\r");
    }
}

#endif

