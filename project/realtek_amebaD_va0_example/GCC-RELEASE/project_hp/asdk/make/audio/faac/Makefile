
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/faac
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                          Header FILE Directory LIST                         #
#*****************************************************************************#


#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC += $(DIR)/libfaac/aac_util.c
CSRC += $(DIR)/libfaac/aacquant.c
CSRC += $(DIR)/libfaac/backpred.c
CSRC += $(DIR)/libfaac/bitstream.c
CSRC += $(DIR)/libfaac/channels.c
CSRC += $(DIR)/libfaac/faac_func_stubs.c
CSRC += $(DIR)/libfaac/fft.c
CSRC += $(DIR)/libfaac/filtbank.c
CSRC += $(DIR)/libfaac/frame.c
CSRC += $(DIR)/libfaac/huffman.c
CSRC += $(DIR)/libfaac/ltp.c
CSRC += $(DIR)/libfaac/midside.c
CSRC += $(DIR)/libfaac/psychkni.c
CSRC += $(DIR)/libfaac/tns.c
CSRC +=	$(DIR)/faac_api.c
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
	$(AR) rvs lib_faac.a $(OBJS)
	$(MOVE) -f lib_faac.a $(ROOTDIR)/lib/application
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
