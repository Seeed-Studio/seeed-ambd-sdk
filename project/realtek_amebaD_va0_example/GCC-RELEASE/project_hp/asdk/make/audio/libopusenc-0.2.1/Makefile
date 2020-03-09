
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/libopusenc-0.2.1
DIR_LIB1 = $(BASEDIR)/component/common/audio/opus-1.3.1
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/src/ogg_packer.c
CSRC += $(DIR)/src/opus_header.c
CSRC += $(DIR)/src/opusenc.c
#CSRC += $(DIR)/src/picture.c
CSRC += $(DIR)/src/resample.c
CSRC += $(DIR)/src/unicode_support.c



MODULE_IFLAGS += -I$(DIR)/include
MODULE_IFLAGS += -I$(DIR_LIB1)/include
#MODULE_IFLAGS += -I$(DIR)/win32
GLOBAL_CFLAGS += -DOUTSIDE_SPEEX
#GLOBAL_CFLAGS += -DRANDOM_PREFIX=3
GLOBAL_CFLAGS += -DHAVE_CONFIG_H
#GLOBAL_CFLAGS += -DHAVE_ALLOCA_H
GLOBAL_CFLAGS += -DDISABLE_FLOAT_API
#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))
STATIC_LIB = lib_opusenc.a
#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CORE_TARGETS COPY_RAM_OBJS

#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(OBJS) $(STATIC_LIB)
$(STATIC_LIB):$(OBJS)
	$(AR) rvs $@ $^
	$(MOVE) -f $@ $(ROOTDIR)/lib/application

#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)