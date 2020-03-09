##################################################################################
#                                                                                #
#                             Example example_usbh_msc                           #
#                                                                                #
##################################################################################

Date: 2019-12-06

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Parameter Setting and Configuration
 - Result description
 - Other Reference
 - Supported List

 
Description
~~~~~~~~~~~
        With USB host v2.0 interface, Ameba can be designed to a USB mass storage host.
        In this application, Ameba boots up as USB mass storage host, detects USB mass storage device 
        attached to it and do basic transfer performance test via FATFS.
    
Setup Guide
~~~~~~~~~~~
        In order to run this application successfully, the hardware setup should be confirm before moving on.
        1. Connect USB mass storage device (i.e. UDisk or USB card reader with microSD card attached) to the micro USB
           connector on Ameba develop board.
        2. For GCC environmnet, type command "make menuconfig" and choose CONFIG USB => Enable USB => USB OTG Type => 
           USB_OTG_HOST_MODE and then choose USB Host Type => USB_HOST_MSC,
           this will auto-generate following lines in platform_autoconf.h:
               #define CONFIG_USB_OTG_EN              1
               #define CONFIG_USB_HOST_EN             1
               #define CONFIG_USBH_MSC                1
           And for IAR environment, manually edit platform_autoconf.h as above.
        3. Make sure the USBH MSC example and FATFS USB disk interface are enabled (default enabled only if 
           CONFIG_USBH_MSC defined) in platform_opts.h:
               #if defined(CONFIG_USBH_MSC)
               #define CONFIG_EXAMPLE_USBH_MSC        1
               #if CONFIG_EXAMPLE_USBH_MSC
               #define CONFIG_FATFS_EN	              1
               #if CONFIG_FATFS_EN
               // fatfs version
               #define FATFS_R_10C
               // fatfs disk interface
               #define FATFS_DISK_USB	              1
               #define FATFS_DISK_SD 	              0
               #define FATFS_DISK_FLASH 	          0
               #endif
               #endif
               #endif
        4. Make sure the proper configurations of FATFS in ffconf.h:
               #define  _MIN_SS     512   // Define the minimum sector size supported.
               #define  _MAX_SS     512   // Define the maximum sector size supported.
		       #define  _USE_MKFS   0     // Disable f_mkfs() function.
        5. Specially for IAR environment, manually include lib/lib_usbh.a (default excluded) to and exclude lib/lib_usbd.a
           (default included) from the build of km4_application project.
        6. Rebuild the project and download firmware to Ameba develop board.
        7. Reset and check the log via serial console.

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Adjust the application test buffer size or test rounds as needed:
               #define USBH_MSC_TEST_SIZE         (512 * 8)
               #define USBH_MSC_TEST_ROUNDS        20
    
Result description
~~~~~~~~~~~~~~~~~~
        On the serial console, USB mass storage host loading log and transfer performance testing log will be printed, 
        make sure there is no error reported.
        The application runs only for once, reset the Ameba develop board to run it again if needed.

Other Reference
~~~~~~~~~~~~~~~
        None.

Supported List
~~~~~~~~~~~~~~
[Supported List]
        Supported : Ameba-D
        Source code not in project: Ameba-1, Ameba-z, Ameba-pro