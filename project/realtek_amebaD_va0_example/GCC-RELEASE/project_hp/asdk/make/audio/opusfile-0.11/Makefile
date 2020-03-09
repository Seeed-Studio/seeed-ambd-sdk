
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/opusfile-0.11
DIR_OGG =  $(BASEDIR)/component/common/audio/libogg-1.3.3
DIR_OPUS = $(BASEDIR)/component/common/audio/opus-1.3.1
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
#CSRC = $(DIR)/src/http.c
CSRC += $(DIR)/src/info.c
CSRC += $(DIR)/src/internal.c
CSRC += $(DIR)/src/opusfile.c
CSRC += $(DIR)/src/stream.c
CSRC += $(DIR)/src/wincerts.c

MODULE_IFLAGS += -I$(DIR)/src
MODULE_IFLAGS += -I$(DIR)/include
MODULE_IFLAGS += -I$(DIR)/win32

MODULE_IFLAGS += -I$(DIR_OGG)/include
MODULE_IFLAGS += -I$(DIR_OPUS)/include
#GLOBAL_CFLAGS += -DHAVE_CONFIG_H
GLOBAL_CFLAGS += -DHAVE_ALLOCA_H
GLOBAL_CFLAGS += -DDISABLE_FLOAT_API
GLOBAL_CFLAGS += -DOP_FIXED_POINT
#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))
STATIC_LIB = lib_opusfile.a
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