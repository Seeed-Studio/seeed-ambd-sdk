#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_USBD_CDC_ACM) && CONFIG_EXAMPLE_USBD_CDC_ACM
#include <platform/platform_stdlib.h>
#include "usb.h"
#include "usbd_cdc_acm_if.h"
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
#define CONFIG_USDB_CDC_CHECK_USB_STATUS   1

#define ACM_BULK_IN_BUF_SIZE  256
#define ACM_BULK_OUT_BUF_SIZE 256

static int acm_init(void)
{
    usbd_cdc_acm_set_bulk_transfer_buffer_size(ACM_BULK_OUT_BUF_SIZE, ACM_BULK_IN_BUF_SIZE);
    return ESUCCESS;
}

static int acm_deinit(void)
{
    return ESUCCESS;
}

static int acm_receive(void *buf, u16 length)
{
    u16 len = (length > ACM_BULK_OUT_BUF_SIZE) ? ACM_BULK_OUT_BUF_SIZE : length;
    // Echo the received message back to host
    usbd_cdc_acm_transmit_data(buf, len);
    return ESUCCESS;
}

usbd_cdc_acm_usr_cb_t cdc_acm_usr_cb = {
    .init = acm_init,
    .deinit = acm_deinit,
    .receive = acm_receive,
};
    
#if CONFIG_USDB_CDC_CHECK_USB_STATUS
static void cdc_check_usb_status_thread(void *param)
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
                usbd_cdc_acm_deinit();
                usb_deinit();
                ret = usb_init(USB_SPEED_HIGH);
                if (ret != 0) {
                    printf("\n\rUSB re-init fail\n\r");
                    break;
                }
                ret = usbd_cdc_acm_init(&cdc_acm_usr_cb);
                if (ret != 0) {
                    printf("\n\rUSB CDC ACM re-init fail\n\r");
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

static void example_usbd_cdc_acm_thread(void *param)
{
    int ret = 0;
#if CONFIG_USDB_CDC_CHECK_USB_STATUS
    struct task_struct task;
#endif
    
    UNUSED(param);
    
    ret = usb_init(USB_SPEED_HIGH);
    if (ret != 0) {
        printf("\n\rUSB init fail\n\r");
        goto example_usbd_cdc_acm_thread_fail;
    }

    ret = usbd_cdc_acm_init(&cdc_acm_usr_cb);
    if (ret != 0) {
        printf("\n\rUSB CDC ACM init fail\n\r");
        usb_deinit();
        goto example_usbd_cdc_acm_thread_fail;
    }
    
#if CONFIG_USDB_CDC_CHECK_USB_STATUS
    ret = rtw_create_task(&task, "cdc_check_usb_status_thread", 512, tskIDLE_PRIORITY + 2, cdc_check_usb_status_thread, NULL);
    if (ret != pdPASS) {
        printf("\n\rUSBD CDC ACM create USB status check thread fail\n\r");
        usbd_cdc_acm_deinit();
        usb_deinit();
        goto example_usbd_cdc_acm_thread_fail;
    }
#endif // CONFIG_USDB_CDC_CHECK_USB_STATUS

    rtw_mdelay_os(100);
    
    printf("\n\rUSBD CDC ACM demo started\n\r");
    
example_usbd_cdc_acm_thread_fail:

    rtw_thread_exit();
}

void example_usbd_cdc_acm(void)
{
    int ret;
    struct task_struct task;
    
    ret = rtw_create_task(&task, "example_usbd_cdc_acm_thread", 1024, tskIDLE_PRIORITY + 5, example_usbd_cdc_acm_thread, NULL);
    if (ret != pdPASS) {
        printf("\n\rUSBD CDC ACM create thread fail\n\r");
    }
}

#endif

