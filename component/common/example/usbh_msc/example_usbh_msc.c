#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_USBH_MSC) && CONFIG_EXAMPLE_USBH_MSC
#include <platform/platform_stdlib.h>
#include "usb.h"
#include "osdep_service.h"
#include "ff.h"
#include "fatfs_ext/inc/ff_driver.h"
#include "disk_if/inc/usbdisk.h"

#define USBH_MSC_THREAD_STACK_SIZE  2048
#define USBH_MSC_TEST_BUF_SIZE      4096
#define USBH_MSC_TEST_ROUNDS        20
#define USBH_MSC_TEST_SEED          0xA5

void example_usbh_msc_thread(void *param)
{
    FATFS fs;
    FIL f;
    int drv_num = 0;
    FRESULT res;
    char logical_drv[4];
    char path[64];
    char filename[64] = "TEST.DAT";
    int ret = 0;
    u32 br;
    u32 bw;
    u32 round = 0;
    u32 start;
    u32 elapse;
    u32 perf = 0;
    u32 test_sizes[] = {512, 1024, 2048, 4096, 8192};
    u32 test_size;
    u32 i;
    u8 *buf = NULL;

    UNUSED(param);

    buf = (u8 *)rtw_malloc(USBH_MSC_TEST_BUF_SIZE);
    if (buf == NULL) {
        printf("\nFail to allocate USBH MSC test buffer\n");
        goto exit;
    }
    
    ret = usb_init();
    if (ret != USB_INIT_OK) {
        printf("\nFail to init USB\n");
        goto exit_free;
    }
    
    // Register USB disk driver to fatfs
    printf("\nRegister USB disk driver\n");
    drv_num = FATFS_RegisterDiskDriver(&USB_disk_Driver);
    if (drv_num < 0) {
        printf("Fail to register USB disk driver\n");
        goto exit_deinit;
    }

    logical_drv[0] = drv_num + '0';
    logical_drv[1] = ':';
    logical_drv[2] = '/';
    logical_drv[3] = 0;
    
    printf("FatFS USB Write/Read performance test started...\n");
    
    // mount fatfs
    res = f_mount(&fs, logical_drv, 1);
    if (res) {
        printf("Fail to mount logical drive: %d\n", res);
        goto exit_unregister;
    }

    strcpy(path, logical_drv);
    sprintf(&path[strlen(path)], "%s", filename);

    // open test file
    printf("Test file: %s\n\n", filename);
    res = f_open(&f, path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if (res) {
        printf("Fail to open file: %s\n", filename);
        goto exit_unmount;
    }

    // clean write and read buffer
    memset(&buf[0], USBH_MSC_TEST_SEED, USBH_MSC_TEST_BUF_SIZE);

    for (i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
        test_size = test_sizes[i];
        if (test_size > USBH_MSC_TEST_BUF_SIZE) {
            break;
        }

        printf("Write test: size = %d round = %d...\n", test_size, USBH_MSC_TEST_ROUNDS);
        start = SYSTIMER_TickGet();

        for (round = 0; round < USBH_MSC_TEST_ROUNDS; ++round) {
            res = f_write(&f, (void *)buf, test_size, (UINT *)&bw);
            if (res || (bw < test_size)) {
                f_lseek(&f, 0);
                printf("Write error bw=%d, rc=%d\n", bw, res);
                ret = 1;
                break;
            }
        }

        elapse = SYSTIMER_GetPassTime(start);
        perf = (round * test_size * 10000 / 1024) / elapse;
        printf("Write rate %d.%d KB/s for %d round @ %d ms\n", perf / 10, perf % 10, round, elapse);
        
        /* move the file pointer to the file head*/
        res = f_lseek(&f, 0);
        
        printf("Read test: size = %d round = %d...\n", test_size, USBH_MSC_TEST_ROUNDS);
        start = SYSTIMER_TickGet();

        for (round = 0; round < USBH_MSC_TEST_ROUNDS; ++round) {
            res = f_read(&f, (void *)buf, test_size, (UINT *)&br);
            if (res || (br < test_size)) {
                f_lseek(&f, 0);
                printf("Read error br=%d, rc=%d\n", br, res);
                ret = 1;
                break;
            }
        }

        elapse = SYSTIMER_GetPassTime(start);
        perf = (round * test_size * 10000 / 1024) / elapse;
        printf("Read rate %d.%d KB/s for %d round @ %d ms\n", perf / 10, perf % 10, round, elapse);
        
        /* move the file pointer to the file head*/
        res = f_lseek(&f, 0);
    }
    
    printf("FatFS USB Write/Read performance test %s\n", (ret == 0) ? "done" : "aborted");

    // close source file
    res = f_close(&f);
    if (res) {
        printf("Fail to close file: %s\n", filename);
    }

exit_unmount:
    if (f_mount(NULL, logical_drv, 1) != FR_OK) {
        printf("Fail to unmount logical drive\n");
    }
exit_unregister:
    if (FATFS_UnRegisterDiskDriver(drv_num)) {
        printf("Fail to unregister disk driver from FATFS\n");
    }
exit_deinit:
    usb_deinit();
exit_free:
    if (buf) {
        rtw_free(buf);
    }
exit:
    rtw_thread_exit();
}

void example_usbh_msc(void)
{
    int ret;
    struct task_struct task;
    
    printf("\nUSB host MSC demo started...\n");

    ret = rtw_create_task(&task, "example_usbh_msc_thread", USBH_MSC_THREAD_STACK_SIZE, tskIDLE_PRIORITY + 2, example_usbh_msc_thread, NULL);
    if (ret != pdPASS) {
        printf("\n%Fail to create USB host MSC thread\n");
    }
}

#endif
