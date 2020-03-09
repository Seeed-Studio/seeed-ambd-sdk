
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/flac

vpath %.c $(DIR) $(shell find $(DIR) -type d)
vpath %.S $(DIR) $(shell find $(DIR) -type d)
#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/bitstream.c \
		$(DIR)/decoder.c	\
		$(DIR)/tables.c
	
	
ASMSRC = $(DIR)/arm.S
	
	
	

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))
ASM_OBJS = $(notdir $(ASMSRC:.S=.o))
STATIC_LIB = lib_flac.a
#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
#all: CORE_TARGETS
all: CORE_TARGETS
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS) $(ASM_OBJS) $(STATIC_LIB)
#CORE_TARGETS: $(OBJS)
$(STATIC_LIB):$(OBJS) $(ASM_OBJS)
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
