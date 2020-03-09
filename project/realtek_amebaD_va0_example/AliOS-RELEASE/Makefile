# Realtek Semiconductor Corp.
# Build libraries for AliOS SDK

all: clean lib_all

OS := $(shell uname)

.PHONY: toolchain
toolchain:
ifeq ($(OS),Linux)
	if [ ! -d toolchain/cygwin/asdk-6.4.1 ] ; then tar -jxvf ../GCC-RELEASE/project_hp/toolchain/asdk/asdk-6.4.1-cygwin-newlib-build-2778-i686.tar.bz2 -C  toolchain/cygwin/ ; fi
else
	if [ ! -d toolchain/cygwin/asdk-6.4.1 ] ; then tar -jxvf ../GCC-RELEASE/project_hp/toolchain/asdk/asdk-6.4.1-cygwin-newlib-build-2778-i686.tar.bz2 -C  toolchain/cygwin/ ; fi
endif


.PHONY: lib_all
lib_all: toolchain
#	@$(MAKE) -f lib_wps.mk
#	@$(MAKE) -f lib_p2p.mk
	@$(MAKE) -f lib_wlan.mk

.PHONY: clean
clean:
	@$(MAKE) -f lib_wlan.mk clean
