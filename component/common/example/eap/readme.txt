802.1X EAP METHOD SUPPLICANT EXAMPLE

Description:
Use 802.1X EAP methods to connect to AP and authenticate with backend radius server.
Current supported methods are EAP-TLS, PEAPv0/EAP-MSCHAPv2, and EAP-TTLS/MSCHAPv2.

Configuration:
Modify the argument of example_eap() in example_entry.c to set which EAP methods you want to use.
[example_entry.c]
#if defined(CONFIG_EXAMPLE_EAP) && CONFIG_EXAMPLE_EAP
	example_eap("tls");
	//example_eap("peap");
	//example_eap("ttls");
#endif

Modify the connection config (ssid, identity, password, cert) in example_eap_config() of example_eap.c based on your server's setting.
[FreeRTOSConfig.h]
	#define configTOTAL_HEAP_SIZE          ( ( size_t ) ( 70 * 1024 ) )
[platform_opts.h]
	# define CONFIG_EXAMPLE_EAP	1

	// Turn on/off the specified method
	# define CONFIG_ENABLE_PEAP	1
	# define CONFIG_ENABLE_TLS	1
	# define CONFIG_ENABLE_TTLS	1

	// If you want to verify the certificate of radius server, turn this on
	# define ENABLE_EAP_SSL_VERIFY_SERVER	1

Add lib_eap.a to project and modify makefile/IAR project setting to link it
	(a) For IAR project
			lib_eap.a is located at component/soc/realtek/amebad/misc/bsp/lib/common/IAR, add it to group <lib> and rebuild the project
	(b) For GCC project
			For AmebaD, lib_eap.a is located at project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/lib/application, modify makefile to link it, such as LINK_APP_LIB += $(ROOTDIR)/lib/application/lib_eap.a

Execution:
An EAP connection thread will be started automatically when booting.

Note:
Please make sure the lib_wps, polarssl/mbedtls, ssl_ram_map are also builded.

If the connection failed, you can try the following directions to make it work:
1. Make sure the config_rsa.h of PolarSSL/MbedTLS include the SSL/TLS cipher suite supported by radius server.
2. Make sure the config_rsa.h of PolarSSL/MbedTLS include the certificate format which you use.
3. For EAP-TLS authentication, may need to enable POLARSSL_DES_C or MBEDTLS_DES_C in config_rsa.h to support for encrypted private key which you set in void example_eap_config(void).
4. Set a larger value to SSL_MAX_CONTENT_LEN in config_rsa.h
5. Increase the FreeRTOS heap in FreeRTOSConfig.h, for example you can increase the heap to 80kbytes:
	#define configTOTAL_HEAP_SIZE	( ( size_t ) ( 80 * 1024 ) )
6. Try to change using SW crypto instead of HW crypto.


[Supported List]
	Supported :
	    Ameba-1, Ameba-z, Ameba-pro, Ameba-D
