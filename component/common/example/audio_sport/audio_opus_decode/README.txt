This opus decode example is used to play opus files from the flash. In order to run the example the following steps must be followed:

    1. Set the parameter CONFIG_EXAMPLE_AUDIO_OPUS_DECODE to 1 in project/realtek_amebaD_va0_example/inc/inc_hp/platform_opts.h file
       
    2. In Cygwin terminal, change to the directory ”project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp”, 
       type ”make menuconfig”, and enable audio related configurations (MENUCONFIG FOR CHIP CONFIG -> Audio Config -> Enable Audio).
       
    3. The file to be played is "ready_to_convert0.h", which containes 1000Hz audio data in ogg container.
       
    4. Build and flash the binary to test


[Supported List]
	Supported :
	    Ameba-D
	Source code not in project:

[Compilation Tips]
    1. When WI-FI and Network are off,   you need to call “example_audio_opus_decode()” on your own because the “example_entry()” function will not be called automatically. In this situation, 
	   the program may not run in a normal way due to the default small heap size. You can set configTOTAL_HEAP_SIZE to ( ( size_t ) ( 250 * 1024 ) ) to make sure that ram allocated is enough to run this example(refer to project/realtek_amebaD_va0_example/inc/inc_hp/FreeRTOSConfig.h).
       
    2. This example needs a big memory space, or hardfault may happen. You can choose to use RAM only or RAM with PSRAM to run this example.
       In "/component/common/audio/libogg-1.3.3/include/ogg/os_types.h", you can choose either mode by changing macro values on line 20, 21.
       If you "#define RAM  1", set configTOTAL_HEAP_SIZE to ( ( size_t ) ( 400 * 1024 ) ) is enough to run this example.
       If you "#define PSRAM 1", set configTOTAL_HEAP_SIZE to ( ( size_t ) ( 250 * 1024 ) ) is enough to run this example. In this case, remember to enable psram use in rtl8721dhp_intfcfg.c. Since our ram is not big enough, we advise to use PSRAM.

	3. In IAR, you need to include lib_opus.a lib_libogg.a lib_opusfile.a under "km4_application/lib" and .c files in this folder under "km4_application/utilities/example".