##################################################################################
#                                                                                #
#                           example_wifi_manager                                 #
#                                                                                #
##################################################################################

Date: 2019-11-22

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Result description
 - Supported List

 
Description
~~~~~~~~~~~
        This example shows how to register wifi event callback function.
	
Setup Guide
~~~~~~~~~~~
        In platform_opts.h, please set 	#define CONFIG_EXAMPLE_WIFI_MANAGER 1
	
Result description
~~~~~~~~~~~~~~~~~~
        When define CONFIG_EXAMPLE_WIFI_MANAGER, the callback functiona of wifi events(WIFI_EVENT_NO_NETWORK, WIFI_EVENT_CONNECT, WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, WIFI_EVENT_DISCONNECT) are automatically registered.

Supported List
~~~~~~~~~~~~~~
		Source code not in project :
			Ameba-1, Ameba-z, Ameba-pro, Ameba-D