
# Initialize tool chain
# -------------------------------------------------------------------

ARM_GCC_TOOLCHAIN = toolchain/cygwin/asdk-6.4.1/cygwin/newlib/bin

CROSS_COMPILE = $(ARM_GCC_TOOLCHAIN)/arm-none-eabi-

GLOBAL_CFLAGS += -DDM_ODM_SUPPORT_TYPE=32
# Compilation tools
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
NM = $(CROSS_COMPILE)nm

# Initialize target name and target object files
# -------------------------------------------------------------------

all: CFLAGS += 
all: TARGET=lib_wlan
all: lib_wlan

mp: CFLAGS += -DCONFIG_MP_INCLUDED
mp: TARGET=lib_wlan_mp
mp: lib_wlan_mp

clean: TARGET=lib_wlan
clean_mp: TARGET=lib_wlan_mp

OBJ_DIR=$(TARGET)/Debug/obj
BIN_DIR=$(TARGET)/Debug/bin

# Include folder list
# -------------------------------------------------------------------
DIR = ../../../component/common/drivers/wlan/realtek/src

INCLUDES =
INCLUDES += -I./inc_alios
INCLUDES += -I../../../component/os/alios
INCLUDES += -I../../../component/os/alios/aos
INCLUDES += -I../../../component/os/alios/aos/include
INCLUDES += -I../../../component/os/freertos
INCLUDES += -I../../../component/os/freertos/freertos_v8.1.2/Source/include
INCLUDES += -I../../../component/os/freertos/freertos_v8.1.2/Source/portable/GCC/ARM_CM4F
INCLUDES += -I../../../component/os/os_dep/include
INCLUDES += -I../../../component/common/api/network/include
INCLUDES += -I../../../component/common/api
INCLUDES += -I../../../component/common/api/platform
INCLUDES += -I../../../component/common/api/wifi
INCLUDES += -I../../../component/common/api/wifi/rtw_wpa_supplicant/src
INCLUDES += -I../../../component/common/application/google
INCLUDES += -I../../../component/common/drivers/wlan/realtek/include
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/osdep
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/hci
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/hal
#INCLUDES += -I../../../../component/common/drivers/wlan/realtek/src/hal/OUTSRC
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/hal/rtl8721d
INCLUDES += -I../../../component/common/drivers/wlan/realtek/wlan_ram_map/rom
INCLUDES += -I../../../component/common/network/ssl/ssl_ram_map/rom
INCLUDES += -I../../../component/common/network/ssl/polarssl-1.3.8/include
INCLUDES += -I../../../component/common/network/lwip/lwip_v1.4.1/port/realtek/freertos
INCLUDES += -I../../../component/common/network/lwip/lwip_v1.4.1/src/include
INCLUDES += -I../../../component/common/network/lwip/lwip_v1.4.1/src/include/lwip
INCLUDES += -I../../../component/common/network/lwip/lwip_v1.4.1/src/include/ipv4
INCLUDES += -I../../../component/common/network/lwip/lwip_v1.4.1/port/realtek
#INCLUDES += -I../../../../component/common/drivers/inic/rtl8711b
INCLUDES += -I../../../component/common/utilities
INCLUDES += -I../../../component/sod/stmicro/common
INCLUDES += -I../../../component/soc/realtek/amebad/cmsis
INCLUDES += -I../../../component/soc/realtek/amebad/cmsis/device
INCLUDES += -I../../../component/soc/realtek/amebad/include
INCLUDES += -I../../../component/soc/realtek/amebad/fwlib/include
INCLUDES += -I../../../component/soc/realtek/amebad/misc
INCLUDES += -I../../../component/soc/realtek/amebad/app/monitor/include
INCLUDES += -I../../../component/soc/realtek/amebad/swlib/string
INCLUDES += -I../../../component/soc/realtek/amebad/swlib/include
INCLUDES += -I../../../component/soc/realtek/amebad/app/xmodem
INCLUDES += -I../../../component/common/mbed/api
INCLUDES += -I../../../component/common/mbed/hal
INCLUDES += -I../../../component/common/mbed/hal_ext
INCLUDES += -I../../../component/common/mbed/targets/hal/rtl8721d
INCLUDES += -I../../../component/common/network/ssl/mbedtls-2.4.0/include
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/hal/phydm
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/hal/rtl8721d/little_driver/include
INCLUDES += -I../../../component/os/alios/alios
# Source file list
# -------------------------------------------------------------------

SRC_C =

#Config
SRC_C += $(DIR)/core/rom/rom_rtw_message_e.o \
       $(DIR)/core/rom/rom_rtw_message_f.o \
       $(DIR)/core/rtw_ap.o \
       $(DIR)/core/rtw_debug.o \
       $(DIR)/core/efuse/rtw_efuse.o \
       $(DIR)/core/rtw_ieee80211.o \
       $(DIR)/core/rtw_intfs.o \
       $(DIR)/core/rtw_io.o \
       $(DIR)/core/rtw_ioctl_set.o \
       $(DIR)/core/rtw_mlme.o \
       $(DIR)/core/rtw_mlme_ext.o \
       $(DIR)/core/rtw_mp.o \
       $(DIR)/core/rtw_promisc.o \
       $(DIR)/core/rtw_psk.o \
       $(DIR)/core/rtw_pwrctrl.o \
       $(DIR)/core/rtw_security.o \
       $(DIR)/core/rtw_sta_mgt.o \
       $(DIR)/core/rtw_wlan_util.o \
       $(DIR)/core/rtw_suspend.o \
       $(DIR)/core/rtw_odm.o \
       $(DIR)/core/rtw_cmd.o \
       $(DIR)/core/rtw_btcoex.o \
       $(DIR)/core/rtw_btcoex_wifionly.o \
       $(DIR)/core/rtw_mi.o \
       $(DIR)/core/rtw_rf.o \
       $(DIR)/core/rom/rom_rtw_wlan_util.o \
	   $(DIR)/core/wifi_performance_monitor.o


SRC_C += $(DIR)/hal/phydm/rtl8721d/halhwimg8721d_bb.o \
	$(DIR)/hal/phydm/rtl8721d/halhwimg8721d_mac.o \
	$(DIR)/hal/phydm/rtl8721d/phydm_hal_api8721d.o \
	$(DIR)/hal/phydm/rtl8721d/phydm_rtl8721d.o \
	$(DIR)/hal/phydm/rtl8721d/phydm_regconfig8721d.o

SRC_C +=  \
	$(DIR)/hal/phydm/phydm_debug.o \
	$(DIR)/hal/phydm/phydm_dynamictxpower.o \
	$(DIR)/hal/phydm/phydm_interface.o \
	$(DIR)/hal/phydm/phydm_noisemonitor.o \
	$(DIR)/hal/phydm/phydm_pow_train.o \
	$(DIR)/hal/phydm/phydm_primary_cca.o \
	$(DIR)/hal/phydm/phydm_psd.o \
	$(DIR)/hal/phydm/phydm_rainfo.o \
	$(DIR)/hal/phydm/phydm_rssi_monitor.o \
	$(DIR)/hal/phydm/phydm.o \
	$(DIR)/hal/phydm/phydm_adaptivity.o \
	$(DIR)/hal/phydm/phydm_adc_sampling.o \
	$(DIR)/hal/phydm/phydm_api.o \
	$(DIR)/hal/phydm/phydm_ccx.o \
	$(DIR)/hal/phydm/phydm_cck_pd.o \
	$(DIR)/hal/phydm/phydm_auto_dbg.o \
	$(DIR)/hal/phydm/phydm_dig.o \
	$(DIR)/hal/phydm/phydm_dfs.o \
	$(DIR)/hal/phydm/phydm_antdect.o \
	$(DIR)/hal/phydm/phydm_hwconfig.o
	
SRC_C += $(DIR)/hal/phydm/halrf/halrf.o \
	$(DIR)/hal/phydm/halrf/halrf_debug.o \
	$(DIR)/hal/phydm/halrf/halphyrf_iot.o \
	$(DIR)/hal/phydm/halrf/halrf_kfree.o \
	$(DIR)/hal/phydm/halrf/halrf_powertracking.o \
	$(DIR)/hal/phydm/halrf/halrf_powertracking_iot.o \
	$(DIR)/hal/phydm/halrf/halrf_txgapcal.o \
	$(DIR)/hal/phydm/halrf/rtl8721d/halrf_8721d.o \
	$(DIR)/hal/phydm/halrf/rtl8721d/halhwimg8721d_rf.o \
	$(DIR)/hal/phydm/halrf/rtl8721d/halrf_dpk_8721d.o \
	$(DIR)/hal/phydm/halrf/rtl8721d/halrf_rfk_init_8721d.o \
	$(DIR)/hal/phydm/halrf/rtl8721d/halrf_txgapk_8721d.o \

SRC_C += $(DIR)/hal/rtl8721d/src/lx_bus/lxbus_halinit.o \
	$(DIR)/hal/rtl8721d/src/lx_bus/lxbus_ops.o \
	$(DIR)/hal/rtl8721d/src/lx_bus/lxbus_suspend.o	\
	$(DIR)/hal/rtl8721d/src/lx_bus/rtl8721d_recv.o \
	
	
SRC_C += $(DIR)/hal/rtl8721d/src/Hal8721DPwrSeq.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_cmd.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_dm.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_firmware.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_hal_efuse.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_hal_init.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_mp.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_phycfg.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_rf6052.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_rxdesc.o \
	$(DIR)/hal/rtl8721d/src/rtl8721d_txdesc.o
	
SRC_C += $(DIR)/hal/hal_com.o \
	$(DIR)/hal/hal_com_phycfg.o \
	$(DIR)/hal/hal_intf.o \
	$(DIR)/hal/hal_phy.o  \
	$(DIR)/hal/HalPwrSeqCmd.o \
	$(DIR)/hal/hal_btcoex.o \
	$(DIR)/hal/halbtccommon.o \
	$(DIR)/hal/hal_dm.o	\
	$(DIR)/hal/hal_btcoex_wifionly.o
	
SRC_C +=	$(DIR)/hal/btc/halbtc8721d.o	\
	$(DIR)/hal/btc/halbtc8721dwifionly.o	\
	$(DIR)/hal/btc/halbtc8721d_phy.c	\

SRC_C += $(DIR)/hci/hci_intfs.o \
	$(DIR)/hci/lxbus/lxbus_hci_intf.o \
	$(DIR)/osdep/netdev.o \

SRC_C += $(DIR)/hal/rtl8721d/src/rom/rom_rtl8721d_hal_init.o \
	$(DIR)/hal/rtl8721d/src/rom/rom_rtl8721d_efuse.o \
	$(DIR)/hal/rtl8721d/src/rom/rom_phydm_8721d.o

SRC_C += $(DIR)/../../../../api/wifi/wifi_simple_config_parser.o

SRC_C += $(DIR)/../../../../bluetooth/realtek/sdk/example/bt_config/bt_config_wifi_lib.o

SRC_C += $(DIR)/../../../../bluetooth/realtek/sdk/board/amebad/src/rtl8721d_bt_calibration.o
SRC_C += $(DIR)/../../../../bluetooth/realtek/sdk/board/amebad/src/rtl8721d_bt.o
#SRAM
SRC_C += $(DIR)/core/rtw_xmit.o	\
	$(DIR)/core/rtw_recv.o \
	#$(DIR)/osdep/lwip_intf.o \

SRC_C += $(DIR)/hal/phydm/phydm_cfotracking.o \
	$(DIR)/hal/phydm/phydm_antdiv.o \
	$(DIR)/hal/phydm/phydm_math_lib.o \
	$(DIR)/hal/phydm/phydm_phystatus.o \
	$(DIR)/hal/phydm/phydm_soml.o
	
SRC_C += $(DIR)/hal/rtl8721d/src/lx_bus/rtl8721d_xmit.o
SRC_C += $(DIR)/hal/rtl8721d/src/rtl8721d_rf_onoff.o

#googlenest
#SRC_C += ../../../../component/common/application/google/google_nest.c


#simple_config
#SRC_C += ../../../../component/common/api/wifi/wifi_simple_config_crypto.c
#SRC_C += ../../../../component/common/api/wifi/wifi_simple_config_parser.c


#wlan core
  
#SRC_C += ../../../../component/common/drivers/wlan/realtek/src/core/rom/rom_rtw_message_e.c
SRC_C += $(DIR)/core/rom/rom_aes.o \
	$(DIR)/core/rom/rom_arc4.o \
	$(DIR)/core/rom/rom_md5.o \
	$(DIR)/core/rom/rom_rc4.o \
	$(DIR)/core/rom/rom_sha1.o \
	$(DIR)/core/rom/rom_rtw_ieee80211.o \
	$(DIR)/core/rom/rom_rtw_psk.o \
	$(DIR)/core/rom/rom_rtw_security.o \
	$(DIR)/../wlan_ram_map/rom/rom_wlan_ram_map.o

SRC_C += $(DIR)/../wlan_ram_map/wlan_ram_map.o

# Little driver for DLPS
SRC_C += $(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_phystatus.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_ops.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_cmd.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_halinit.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_recv.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_suspend.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_xmit.o \
	$(DIR)/hal/rtl8721d/little_driver/src/hw/rs8721d_interrupt.o \
	$(DIR)/hal/rtl8721d/little_driver/src/ethernet_rswlan.o \
	$(DIR)/hal/rtl8721d/little_driver/src/rs_intfs.o \
	$(DIR)/hal/rtl8721d/little_driver/src/rs_network.o \

#wlan osdep
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_intfs.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_ioctl.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_isr.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_mp.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_recv.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_skbuff.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/alios_xmit.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/lxbus_intf.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/netdev.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/rtk_wlan_if.c
SRC_C += ../../../component/common/drivers/wlan/realtek/src/osdep/alios/rtk_bt_if.c
#SRC_C += ../../../../component/soc/realtek/amebad/misc/rtl8721d_alios_pmu.c

# Generate obj list
# -------------------------------------------------------------------

SRC_O = $(patsubst %.c,%.o,$(SRC_C))

SRC_C_LIST = $(notdir $(SRC_C))
OBJ_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_C_LIST)))
DEPENDENCY_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.d,$(SRC_C_LIST)))

# Compile options
# -------------------------------------------------------------------

CFLAGS =
CFLAGS += -MD -DCONFIG_PLATFORM_8721D -DDM_ODM_SUPPORT_TYPE=32
#CFLAGS += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g2 -w -Os -Wno-pointer-sign -fno-common -fmessage-length=0  -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-short-enums -DF_CPU=166000000L -std=gnu99 -fsigned-char
#CFLAGS += -mcpu=cortex-m4 -mthumb -g2 -w -Os -Wno-pointer-sign -fno-common -fmessage-length=0  -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-short-enums -DF_CPU=166000000L -std=gnu99 -fsigned-char
CFLAGS += -march=armv8-m.main+dsp  -mthumb  -mcmse  -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -g -gdwarf-3 -nostartfiles -nodefaultlibs -nostdlib -O2 -D__FPU_PRESENT -gdwarf-3 -fstack-usage -fdata-sections -nostartfiles -nostdlib -Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Wno-write-strings -Wno-maybe-uninitialized --save-temps -c -MMD -Wextra
# Compile
# -------------------------------------------------------------------


.PHONY: lib_wlan
lib_wlan: prerequirement $(SRC_O)
	$(AR) rvs $(BIN_DIR)/$(TARGET).a $(OBJ_LIST)
	cp $(BIN_DIR)/$(TARGET).a ./lib_all/$(TARGET).a

#.PHONY: lib_wlan_mp
#lib_wlan_mp: prerequirement $(SRC_O)
#	$(AR) rvs $(BIN_DIR)/$(TARGET).a $(OBJ_LIST)
#	cp $(BIN_DIR)/$(TARGET).a ./lib_all/$(TARGET).a

.PHONY: prerequirement
prerequirement:
	@echo ===========================================================
	@echo Build $(TARGET).a
	@echo ===========================================================
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

$(SRC_O): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	cp $@ $(OBJ_DIR)/$(notdir $@)
	chmod 777 $(OBJ_DIR)/$(notdir $@)

-include $(DEPENDENCY_LIST)

.PHONY: clean
clean:
	rm -rf $(TARGET)
	rm -f $(SRC_O)
	rm -f $(patsubst %.o,%.d,$(SRC_O))
	rm -f $(patsubst %.o,%.u,$(SRC_O))
	rm -f $(patsubst %.o,%.i,$(SRC_O))
	rm -f $(patsubst %.o,%.s,$(SRC_O))
	rm -f $(patsubst %.o,%.su,$(SRC_O))
	rm -f *.i
	rm -f *.s

#.PHONY: clean_mp
#clean_mp: 
#	rm -rf $(TARGET)
#	rm -f $(SRC_O)
#	rm -f $(patsubst %.o,%.d,$(SRC_O))
