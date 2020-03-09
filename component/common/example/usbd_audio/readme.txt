##################################################################################
#                                                                                #
#                             Example example_usbd_audio                         #
#                                                                                #
##################################################################################

Date: 2019-09-24

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
        With USB device v2.0 interface, Ameba can be designed to a USB Audio class device.
        In this application, Ameba boots up as a USB Audio device, USB host (e.g. windows machine)
        can recognize Ameba and communicates with it via USB interface.
    
Setup Guide
~~~~~~~~~~~
        In order to run this application successfully, the hardware setup should be confirm before moving on.
        1. Connect Ameba to USB host end with a micro USB cable. Ameba DEV board has designed a micro USB
           connector on board.
        2. For GCC environmnet, type command "make menuconfig" and choose CONFIG USB => Enable USB => USB OTG Type => 
           USB_OTG_DEVICE_MODE and then choose USB Device Type => USB_DEVICE_AUDIO,
           which will auto-generate following lines in platform_autoconf.h:
               #define CONFIG_USB_OTG_EN              1
               #define CONFIG_USB_DEVICE_EN           1
               #define CONFIG_USBD_AUDIO              1
           And for IAR environment, manually edit platform_autoconf.h as above.
        3. Make sure the USB Audio example is enabled (default) in platform_opts.h:
               #define CONFIG_EXAMPLE_USBD_AUDIO      1
        4. Rebuild the project and download firmware to DEV board.

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        None.
    
Result description
~~~~~~~~~~~~~~~~~~
        On the serial console, USB Audio loading log will be printed, make sure there is no error reported.
        After the USB Audio driver is successfully loaded, USB host end will recognize Ameba as an audio device. 
        Now user can record and play audio via the audio device.

Other Reference
~~~~~~~~~~~~~~~
        None

Supported List
~~~~~~~~~~~~~~
[Supported List]
        Supported : Ameba-D
        Source code not in project: Ameba-1, Ameba-z, Ameba-pro