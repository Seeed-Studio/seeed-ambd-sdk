#include <platform_opts.h>
#ifdef CONFIG_USBH_VENDOR

#include "usb.h"
#include "usbh_vendor.h"

volatile int vendor_iso_out_cnt = 0x00;
volatile int vendor_iso_out_complete_callback_cnt = 0;
volatile int vendor_iso_in_cnt = 0;
volatile int vendor_iso_in_cnt_check = 0;
volatile int vendor_iso_in_callback_bit = 1;
volatile int vendor_usb_connect_state = 0; // 1:connected 0:disconnected

#ifdef  CONFIG_USBH_VENDOR_ISO_IN_TEST
struct urb *vendor_iso_in_urb[2];
u8 *vendor_iso_in_buf = NULL;
#endif
#ifdef  CONFIG_USBH_VENDOR_ISO_OUT_TEST
struct urb *vendor_iso_out_urb[2];
u8 *vendor_iso_out_buf = NULL;
#endif

struct usbtest_dev *vendor_dev;
struct usb_device *vendor_udev;
struct usb_interface *vendor_intf;
struct usbtest_info vendor_info;

static inline struct usb_device *vendor_testdev_to_usbdev(struct usbtest_dev *dev)
{
    return interface_to_usbdev(dev->intf);
}

#ifdef CONFIG_USBH_VENDOR_BULK_TEST
static inline void vendor_bulk_complete_callback(struct urb *urb)
{
    usb_free_urb(urb);
}
#endif

#ifdef CONFIG_USBH_VENDOR_CTRL_TEST
static inline void vendor_ctrl_complete_callback(struct urb *urb)
{
    usb_free_urb(urb);
}
#endif

static inline void vendor_update_endpoint(int edi,
    struct usb_host_endpoint **in,
    struct usb_host_endpoint **out,
    struct usb_host_endpoint *e)
{
    if (edi) {
        if (!*in) {
            *in = e;
        }
    } else {
        if (!*out) {
            *out = e;
        }
    }
}

static int vendor_get_endpoints(struct usbtest_dev *dev, struct usb_interface *intf)
{
    unsigned int tmp = 0;
    struct usb_host_interface *alt;
    struct usb_host_endpoint  *in, *out;
    struct usb_host_endpoint  *iso_in, *iso_out;
    struct usb_host_endpoint  *int_in, *int_out;
    struct usb_device *udev;
    
    dev->info = &vendor_info;

    for (tmp = 0; tmp < intf->num_altsetting; tmp++) {
        unsigned ep;
        in = out = NULL;
        iso_in = iso_out = NULL;
        int_in = int_out = NULL;
        alt = intf->altsetting + tmp;

        for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
            struct usb_host_endpoint *e = &alt->endpoint[ep];
            struct usb_endpoint_descriptor *desc = &e->desc;
            int edi;
            edi = usb_endpoint_dir_in(&e->desc);

            switch (usb_endpoint_type(&e->desc)) {
                case USB_ENDPOINT_XFER_BULK:
                    if (edi) {
                        dev->in_pipe_addr = desc->bEndpointAddress;
                    } else {
                        dev->out_pipe_addr = desc->bEndpointAddress;
                    }

                    break;

                case USB_ENDPOINT_XFER_INT:
                    if (dev->info->intr) {
                        vendor_update_endpoint(edi, &int_in, &int_out, e);
                    }

                    break;

                case USB_ENDPOINT_XFER_ISOC:
                    if (edi) {
                        dev->in_iso_addr = desc->bEndpointAddress;
                        dev->iso_in = &e->desc;
                    } else {
                        dev->out_iso_addr = desc->bEndpointAddress;
                        dev->iso_out = &e->desc;
                    }

                    vendor_update_endpoint(edi, &iso_in, &iso_out, e);
                    break;

                default:
                    break;
            }
        }
    }

    if ((in && out)  ||  iso_in || iso_out || int_in || int_out) {
        goto found;
    }

found:
    udev = vendor_testdev_to_usbdev(dev);
    dev->info->alt = alt->desc.bAlternateSetting;

    if (iso_in) {
        dev->iso_in = &iso_in->desc;
        dev->in_iso_pipe = usb_rcvisocpipe(udev,
                iso_in->desc.bEndpointAddress
                & USB_ENDPOINT_NUMBER_MASK);
        USBH_VENDOR_INFO("ISO in endpoint address: 0x%02X\n", iso_in->desc.bEndpointAddress);
    }

    if (iso_out) {
        dev->iso_out = &iso_out->desc;
        dev->out_iso_pipe = usb_sndisocpipe(udev,
                iso_out->desc.bEndpointAddress
                & USB_ENDPOINT_NUMBER_MASK);
        USBH_VENDOR_INFO("ISO out endpoint address: 0x%02X\n", iso_out->desc.bEndpointAddress);
    }

    return 0;
}

#if 0
static int set_altsetting(struct usbtest_dev *dev, int alternate)
{
    struct usb_interface        *iface = dev->intf;
    struct usb_device       *udev;

    if (alternate < 0 || alternate >= 256) {
        return -EINVAL;
    }

    udev = interface_to_usbdev(iface);
    return usb_set_interface(udev,
            iface->altsetting[0].desc.bInterfaceNumber,
            alternate);
}
#endif

#ifdef  CONFIG_USBH_VENDOR_ISO_IN_TEST
static void vendor_iso_in_complete_callback(struct urb *urb)
{
    volatile int temp = 0;
    int ret;
    int i;
    u8 *mem;
    
    ++vendor_iso_in_cnt_check;

    if (vendor_iso_in_callback_bit == 1) {
        ++vendor_iso_in_cnt;

        for (i = 0; i < urb->number_of_packets; ++i) {
            mem = (u8 *)urb->transfer_buffer + urb->iso_frame_desc[i].offset;
            int g = 0;
            temp = 0;

            for (g = 0; g < USBH_VENDOR_ISO_IN_BUF_SIZE; g++) {
                temp = temp + mem[g];
            }

            temp = temp / USBH_VENDOR_ISO_IN_BUF_SIZE;
            if (temp == 255) {
#if USBH_VENDOR_DEBUG
                int iso_in_cnt = vendor_iso_in_cnt;
#endif
                vendor_iso_in_callback_bit = 0;
                USBH_VENDOR_INFO("ISO in done: vendor_iso_in_cnt=%d, vendor_iso_in_cnt_check=%d\n", iso_in_cnt, vendor_iso_in_cnt_check);
                vendor_iso_in_cnt = 0;
                return;
            }

            for (g = 0; g < USBH_VENDOR_ISO_IN_BUF_SIZE; g++) {
                mem[g] = 0;
            }
        }

        ret = usb_submit_urb(urb);
        if (ret < 0) {
            USBH_VENDOR_ERROR("\nFail to resubmit iso in urb: %d\n", ret);
        }
    }
}
#endif // CONFIG_USBH_VENDOR_ISO_IN_TEST

#ifdef  CONFIG_USBH_VENDOR_ISO_OUT_TEST
static void vendor_iso_out_complete_callback(struct urb *urb)
{
    int ret;
    u8 *buf = rtw_malloc(USBH_VENDOR_ISO_OUT_BUF_SIZE);
    rtw_memset(buf, vendor_iso_out_cnt, USBH_VENDOR_ISO_OUT_BUF_SIZE);
    urb->transfer_buffer = buf ;
    vendor_iso_out_complete_callback_cnt++;

    if (USBH_VENDOR_ISO_IN_CNT <= vendor_iso_out_complete_callback_cnt) {
        USBH_VENDOR_INFO("ISO out done\n");
        rtw_free(buf);
        return; //STOP OUT_TRANSACTION
    }

    if ((ret = usb_submit_urb(urb)) < 0) {
        USBH_VENDOR_ERROR("\nFail to resubmit iso out urb: %d\n", ret);
    }

    vendor_iso_out_cnt++;
    vendor_iso_out_cnt = vendor_iso_out_cnt % 256;
    rtw_free(buf);
}
#endif // CONFIG_USBH_VENDOR_ISO_OUT_TEST

static unsigned int vendor_get_endpoint_max_packet_size(struct usb_device *dev,
    struct usb_endpoint_descriptor *desc)
{
    u16 psize;

    switch (dev->speed) {
        case USB_SPEED_HIGH:
            psize = desc->wMaxPacketSize;
            return (psize & 0x07ff) * (1 + ((psize >> 11) & 3));

        default:
            psize = desc->wMaxPacketSize;
            return psize & 0x07ff;
    }
}

static int vendor_test(struct usb_interface *intf)
{
    vendor_usb_connect_state = 1;
    int i = 0;
    unsigned int j = 0;

    USBH_VENDOR_ENTER;

    vendor_dev = (struct usbtest_dev *)rtw_zmalloc(sizeof * vendor_dev);
    if (NULL == vendor_dev) {
        USBH_VENDOR_ERROR("Fail to allocate vendor device\n");
        return 1;
    }

    vendor_dev->intf = intf;
    usb_set_intfdata(intf, vendor_dev);
    vendor_udev = vendor_testdev_to_usbdev(vendor_dev);
    //set_altsetting(vendor_dev,0);
    
    /**********************************************
     * Get endpoint address                       *
     **********************************************/
    vendor_get_endpoints(vendor_dev, intf);
    
    /********************************************************
     * ctrl setup request: Get device descriptor Request    *
     ********************************************************/
#ifdef CONFIG_USBH_VENDOR_CTRL_TEST
    struct urb *ctrl_urb;
    u8 *ctrl_data_buf;
    u8 *ctrl_setup_buf;
    unsigned int ctrl_pipe;
    int ctrl_ret;
    struct usb_device *udev = interface_to_usbdev(intf);

    USBH_VENDOR_INFO("USBH vendor control transfer test...\n");
    
    ctrl_urb = usb_alloc_urb(0);
    if (NULL == ctrl_urb) {
        USBH_VENDOR_ERROR("Fail to allocate control urb\n");
        goto ctrl_test_exit;
    }
    ctrl_data_buf =  rtw_zmalloc(18);
    if (NULL == ctrl_data_buf) {
        USBH_VENDOR_ERROR("Fail to allocate control data buffer\n");
        goto ctrl_test_exit;
    }
    ctrl_setup_buf = rtw_zmalloc(8);
    if (NULL == ctrl_setup_buf) {
        USBH_VENDOR_ERROR("Fail to allocate control setup buffer\n");
        goto ctrl_test_exit;
    }
    
    ctrl_setup_buf[0] = 0x80; //0xa1;
    ctrl_setup_buf[1] = 0x06; //0xfe;
    ctrl_setup_buf[2] = 0x00;
    ctrl_setup_buf[3] = 0x01; //0;
    ctrl_setup_buf[4] = 0;
    ctrl_setup_buf[5] = 0;
    ctrl_setup_buf[6] = 0x12; //1;
    ctrl_setup_buf[7] = 0;
    /****************************************************************
     * allocate the parameter of control URB,                       *
     * call usb_submit_urb to transmit/receive data                 *
     * if the transmission complete, the return value will be zero, *
     * and the callback function will free URB                      *
     * ctrl_data_buf[0] is used to receive the return value by device        *
     ****************************************************************/
    ctrl_data_buf[0] = 99; //this buf is used to receive the return value by device
    ctrl_pipe = usb_rcvctrlpipe(udev, 0);
    
    usb_fill_control_urb(ctrl_urb, udev, ctrl_pipe, ctrl_setup_buf, ctrl_data_buf, 18, vendor_ctrl_complete_callback, NULL);
    
    ctrl_ret = usb_submit_urb(ctrl_urb);
    if (ctrl_ret) {
        USBH_VENDOR_ERROR("Fail to commit control urb\n");
    }

    rtw_mdelay_os(10);

ctrl_test_exit:
    rtw_free(ctrl_setup_buf);
    usb_free_urb(ctrl_urb);
    rtw_free(ctrl_data_buf);
#endif

#ifdef CONFIG_USBH_VENDOR_BULK_TEST
    /****************************************************************
     * allocate the parameter of bulk URB,                          *
     * call usb_submit_urb to transmit data                         *
     * if the transmission complete, the return value will be zero, *
     * and the callback function will free URB                      *
     * buf[0] is the value we try to send to device, and we expect  *
     * device will send this back later.                            *
     ****************************************************************/
    struct urb *bulk_out_urb;
    struct urb *bulk_in_urb;
    u8 *bulk_out_buf;
    u8 *bulk_in_buf;
    int bulk_ret;
    
    USBH_VENDOR_INFO("USBH vendor bulk transfer test...\n");
    
    bulk_out_urb = usb_alloc_urb(0);
    if (NULL == bulk_out_urb) {
        USBH_VENDOR_ERROR("Fail to allocate bulk out urb\n");
        goto bulk_test_exit;
    }

    bulk_in_urb = usb_alloc_urb(0);
    if (NULL == bulk_in_urb) {
        USBH_VENDOR_ERROR("Fail to allocate bulk in urb\n");
        goto bulk_test_exit;
    }

    bulk_out_buf  = rtw_zmalloc(16);
    if (NULL == bulk_out_buf) {
        USBH_VENDOR_ERROR("Fail to allocate bulk out buffer\n");
        goto bulk_test_exit;
    }

    bulk_in_buf = rtw_zmalloc(16);
    if (NULL == bulk_in_buf) {
        USBH_VENDOR_ERROR("Fail to allocate bulk in buffer\n");
        goto bulk_test_exit;
    }

    bulk_out_buf[0] = 102;
    
    usb_fill_bulk_urb(bulk_out_urb, vendor_udev, usb_sndbulkpipe(vendor_udev, vendor_dev->out_pipe_addr), bulk_out_buf, 16, vendor_bulk_complete_callback, NULL);
    bulk_ret = usb_submit_urb(bulk_out_urb);
    if (bulk_ret) {
        USBH_VENDOR_ERROR("Fail to commit bulk out urb: %d\n", bulk_ret);
        goto bulk_test_exit;
    } else {
        USBH_VENDOR_INFO("Actual bulk out data length = %d, D0 = %d\n", bulk_out_urb->actual_length, bulk_out_buf[0]);
    }
    
    rtw_mdelay_os(10);
    
    /*********************************************
     * Print the result                           *
     * bulk_in_buf[0] should be buf[0]+1                 *
     **********************************************/
    /****************************************************************
    * allocate the parameter of control URB,                        *
    * call usb_submit_urb to receive data                           *
    * if the transmission complete, the return value will be zero,  *
    * and the callback function will free URB                       *
    * bulk_in_buf is the value device try to send back, the value should be*
    * incremented 1                                                 *
    *****************************************************************/
    bulk_in_buf[0] = 100;
    
    usb_fill_bulk_urb(bulk_in_urb, vendor_udev, usb_rcvbulkpipe(vendor_udev, vendor_dev->in_pipe_addr), bulk_in_buf, 16, vendor_bulk_complete_callback, NULL);
    bulk_ret = usb_submit_urb(bulk_in_urb);
    if (bulk_ret) {
        USBH_VENDOR_ERROR("Fail to commit bulk out urb: %d\n", bulk_ret);
        goto bulk_test_exit;
    } else {
        USBH_VENDOR_INFO("Actual bulk in data length = %d, D0 = %d\n", bulk_in_urb->actual_length, bulk_in_buf[0]);
    }
    
    rtw_mdelay_os(10);

bulk_test_exit:
    usb_free_urb(bulk_out_urb);
    usb_free_urb(bulk_in_urb);
    rtw_free(bulk_out_buf);
    rtw_free(bulk_in_buf);
#endif

    rtw_mdelay_os(300);

#ifdef  CONFIG_USBH_VENDOR_ISO_IN_TEST
    unsigned int iso_in_packet_cnt = 1;
    u16 iso_in_packet_size = vendor_get_endpoint_max_packet_size(vendor_udev, vendor_dev->iso_in);
    vendor_iso_in_callback_bit = 1;
    vendor_iso_in_buf = rtw_malloc(iso_in_packet_cnt  * iso_in_packet_size);
    if (NULL == vendor_iso_in_buf) {
        USBH_VENDOR_ERROR("Fail to allocate iso in buffer\n");
        goto iso_in_test_exit_fail;
    }

    rtw_memset((void *)vendor_iso_in_buf, 0xFF, iso_in_packet_cnt * iso_in_packet_size);

    for (i = 0; i < 1; ++i) {
        vendor_iso_in_urb[i] = usb_alloc_urb(iso_in_packet_cnt);
        if (NULL == vendor_iso_in_urb[i]) {
            USBH_VENDOR_ERROR("Fail to allocate iso in urb\n");
            goto iso_in_test_exit_fail;
        }
        vendor_iso_in_urb[i]->dev = vendor_udev;
        vendor_iso_in_urb[i]->pipe = usb_rcvisocpipe(vendor_udev, vendor_dev->in_iso_addr);
        vendor_iso_in_urb[i]->transfer_flags = URB_ISO_ASAP;//  | URB_NO_TRANSFER_DMA_MAP;
        vendor_iso_in_urb[i]->interval = vendor_dev->iso_out->bInterval;
        vendor_iso_in_urb[i]->transfer_buffer = vendor_iso_in_buf;
        vendor_iso_in_urb[i]->complete = vendor_iso_in_complete_callback;
        vendor_iso_in_urb[i]->context = NULL;
        vendor_iso_in_urb[i]->number_of_packets = iso_in_packet_cnt ;
        vendor_iso_in_urb[i]->transfer_buffer_length = iso_in_packet_cnt * iso_in_packet_size; //12240;

        for (j = 0; j < iso_in_packet_cnt; ++j) {
            vendor_iso_in_urb[i]->iso_frame_desc[j].offset = j * iso_in_packet_size;
            vendor_iso_in_urb[i]->iso_frame_desc[j].length = iso_in_packet_size;
        }
    }

    for (i = 0; i < 1; ++i) {
        int status = usb_submit_urb(vendor_iso_in_urb[i]);
        if (status < 0) {
            USBH_VENDOR_ERROR("Fail to commit iso in urb: %d\n", status);
            goto iso_in_test_exit_fail;
        }
    }

    goto iso_in_test_exit;

iso_in_test_exit_fail:
    for (i = 0; i < 1; ++i) {
        if (vendor_iso_in_urb[i]) {
            usb_free_urb(vendor_iso_in_urb[i]);
        }
    }
    if (vendor_iso_in_buf) {
        rtw_free(vendor_iso_in_buf);
    }

iso_in_test_exit:
    rtw_mdelay_os(10);
    
#endif

#ifdef  CONFIG_USBH_VENDOR_ISO_OUT_TEST
    unsigned int iso_out_packet_cnt = 1;
    u16  iso_out_packet_size = 64; //vendor_get_endpoint_max_packet_size(vendor_udev,vendor_dev->iso_out);
    
    vendor_iso_out_buf = rtw_malloc(iso_out_packet_cnt * iso_out_packet_size);
    if (NULL == vendor_iso_out_buf) {
        USBH_VENDOR_ERROR("Fail to allocate iso out buffer\n");
        goto iso_out_test_exit_fail;
    }
    
    rtw_memset(vendor_iso_out_buf, vendor_iso_out_cnt++, iso_out_packet_cnt * iso_out_packet_size);

    for (i = 0; i < 1; ++i) {
        vendor_iso_out_urb[i] = usb_alloc_urb(iso_out_packet_cnt);
        if (NULL == vendor_iso_out_urb[i]) {
            USBH_VENDOR_ERROR("Fail to allocate iso out urb\n");
            goto iso_out_test_exit_fail;
        }
        vendor_iso_out_urb[i]->dev = vendor_udev;
        vendor_iso_out_urb[i]->pipe = usb_sndisocpipe(vendor_udev, vendor_dev->out_iso_addr);
        vendor_iso_out_urb[i]->transfer_flags = URB_ISO_ASAP; //  | URB_NO_TRANSFER_DMA_MAP;
        vendor_iso_out_urb[i]->interval = vendor_dev->iso_out->bInterval;
        vendor_iso_out_urb[i]->transfer_buffer = vendor_iso_out_buf;
        vendor_iso_out_urb[i]->complete = vendor_iso_out_complete_callback;
        vendor_iso_out_urb[i]->context = NULL;
        vendor_iso_out_urb[i]->number_of_packets = iso_out_packet_cnt;
        vendor_iso_out_urb[i]->transfer_buffer_length = iso_out_packet_cnt * iso_out_packet_size;

        for (j = 0; j < iso_out_packet_cnt; ++j) {
            vendor_iso_out_urb[i]->iso_frame_desc[j].offset = j * iso_out_packet_size;
            vendor_iso_out_urb[i]->iso_frame_desc[j].length = iso_out_packet_size;
        }
    }

    for (i = 0; i < 1; ++i) {
        int status = usb_submit_urb(vendor_iso_out_urb[i]);
        if (status < 0) {
            USBH_VENDOR_ERROR("Fail to commit iso out urb: %d\n", status);
            goto iso_out_test_exit_fail;
        }
    }

    goto iso_out_test_exit;
    
iso_out_test_exit_fail:
    for (i = 0; i < 1; ++i) {
        if (vendor_iso_out_urb[i]) {
            usb_free_urb(vendor_iso_out_urb[i]);
        }
    }
    if (vendor_iso_out_buf) {
        rtw_free(vendor_iso_out_buf);
    }

iso_out_test_exit:
    goto vendor_test_exit;
    
#endif

vendor_test_exit:
    USBH_VENDOR_EXIT;
    return 0;
}

void vendor_probe_thread(void *param)
{
    struct usb_interface *intf = (struct usb_interface *)param;
    vendor_intf = (struct usb_interface *)param;

    if ((intf != NULL) && (intf->cur_altsetting->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC)) {
        vendor_test(intf);
    }

    rtw_thread_exit();
}

static int vendor_probe(struct usb_interface *intf)
{
    int ret;
    struct task_struct task;

    USBH_VENDOR_ENTER;
    
    ret = rtw_create_task(&task, "vendor_probe_thread", 2048, USBH_VENDOR_TASK_PRIORITY, vendor_probe_thread, intf);
    if (ret == pdPASS) {
        ret = 0;
    } else {
        USBH_VENDOR_ERROR("\nFail to create vendor probe thread\n");
        ret = 1;
    }
    
    USBH_VENDOR_EXIT;
    
    return ret;
}

static void vendor_free(void)
{
    if (vendor_usb_connect_state == 1) {
        vendor_usb_connect_state = 0;
        vendor_iso_out_complete_callback_cnt = 0;
        vendor_iso_out_cnt = 0x00;
        vendor_iso_in_cnt    = 0;
        vendor_iso_in_callback_bit = 1;
        vendor_iso_in_cnt_check = 0;

        if (vendor_dev != NULL) {
            rtw_free(vendor_dev);
        }

        if (vendor_dev != NULL) {
            rtw_free(vendor_dev);
        }

        if (vendor_udev != NULL) {
            usb_disable_interface(vendor_udev, vendor_intf, TRUE);
        }

        if (vendor_udev != NULL) {
            rtw_free(vendor_udev);
        }

#ifdef  CONFIG_USBH_VENDOR_ISO_IN_TEST
        for (int i = 0; i < 1; ++i) {
            if (vendor_iso_in_urb[i]) {
                usb_free_urb(vendor_iso_in_urb[i]);
            }
        }
        
        if (vendor_iso_in_buf) {
            rtw_free(vendor_iso_in_buf);
        }
#endif

#ifdef  CONFIG_USBH_VENDOR_ISO_OUT_TEST
        for (int i = 0; i < 1; ++i) {
            if (vendor_iso_out_urb[i]) {
                usb_free_urb(vendor_iso_out_urb[i]);
            }
        }
        
        if (vendor_iso_out_buf) {
            rtw_free(vendor_iso_out_buf);
        }
#endif
    }
}

static void vendor_disconnect(struct usb_interface *intf)
{
    UNUSED(intf);
    USBH_VENDOR_ENTER;
    vendor_free();
    USBH_VENDOR_EXIT;
}

static struct usb_driver vendor_driver = {
    .name = "vendor_driver",
    .probe = vendor_probe,
    .disconnect = vendor_disconnect,
};

void usbh_vendor_deinit(void)
{
    USBH_VENDOR_ENTER;
    vendor_free();
    usb_unregister_class_driver(&vendor_driver);
    USBH_VENDOR_EXIT;
}

int usbh_vendor_init(void)
{
    int ret = 0;
    
    USBH_VENDOR_ENTER;
    
    ret = usb_register_class_driver(&vendor_driver);
    if (ret) {
        usbh_vendor_deinit();
    }
    
    USBH_VENDOR_EXIT;

    return ret;
}

#endif // CONFIG_USBH_VENDOR

