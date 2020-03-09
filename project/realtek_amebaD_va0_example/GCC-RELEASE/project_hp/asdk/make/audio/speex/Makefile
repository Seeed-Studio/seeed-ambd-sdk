
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/speex

vpath %.c $(DIR) $(shell find $(DIR) -type d)
#*****************************************************************************#
#                          Header FILE Directory LIST                         #
#*****************************************************************************#
MODULE_IFLAGS = -I$(DIR)/
MODULE_IFLAGS = -I$(DIR)/speex/

GLOBAL_CFLAGS += -DHAVE_CONFIG_H
#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC += $(DIR)/speex/buffer.c
CSRC += $(DIR)/speex/fftwrap.c
CSRC += $(DIR)/speex/filterbank.c
CSRC += $(DIR)/speex/jitter.c
CSRC += $(DIR)/speex/kiss_fft.c
CSRC += $(DIR)/speex/kiss_fftr.c
CSRC += $(DIR)/speex/mdf.c
CSRC += $(DIR)/speex/preprocess.c
CSRC += $(DIR)/speex/resample.c
CSRC += $(DIR)/speex/scal.c
CSRC += $(DIR)/speex/smallft.c
#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))
#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
#all: CORE_TARGETS
all: CORE_TARGETS COPY_RAM_OBJS
	$(AR) rvs lib_speex.a $(OBJS)
	$(MOVE) -f lib_speex.a $(ROOTDIR)/lib/application
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS)

#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS 
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)
