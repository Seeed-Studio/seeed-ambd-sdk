##################################################################################
#                                                                                #
#                             Example example_usbd_vendor                        #
#                                                                                #
##################################################################################

Date: 2019-12-16

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
        With USB device v2.0 interface, Ameba can be designed to a USB vendor specific device.
        In this application, Ameba boots up as USB device which can communicate with vendor specific USB host as 
        defined in usbh_vendor application.
    
Setup Guide
~~~~~~~~~~~
        In order to run this application successfully, the hardware setup should be confirm before moving on.
        1. Connect the Ameba develop board to Ameba USB vendor specific host (running usbh_vendor application).
        2. For GCC environmnet, type command "make menuconfig" and choose CONFIG USB => Enable USB => USB OTG Type => 
           USB_OTG_DEVICE_MODE and then choose USB Device Type => USB_DEVICE_VENDOR,
           this will auto-generate following lines in platform_autoconf.h:
               #define CONFIG_USB_OTG_EN              1
               #define CONFIG_USB_DEVICE_EN           1
               #define CONFIG_USBD_VENDOR             1
           And for IAR environment, manually edit platform_autoconf.h as above.
        3. Make sure the USBD VENDOR example is enabled (default enabled only if CONFIG_USBD_VENDOR defined) in platform_opts.h:
               #if defined(CONFIG_USBD_VENDOR)
               #define CONFIG_EXAMPLE_USBD_VENDOR     1
               #endif
        4. Rebuild the project and download firmware to Ameba develop board.
        5. Reset and check the log via serial console.

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        None.
    
Result description
~~~~~~~~~~~~~~~~~~
        On the serial console, USB vendor specific device loading log and vendor specific test record will be printed, 
        make sure there is no error reported.

Other Reference
~~~~~~~~~~~~~~~
        None.

Supported List
~~~~~~~~~~~~~~
[Supported List]
        Supported : Ameba-D
        Source code not in project: Ameba-1, Ameba-z, Ameba-pro