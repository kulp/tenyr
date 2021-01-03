TOP := ..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib

vpath %.tas pass_compile/

get_obj_name = $(notdir $(patsubst %.tas,%.to,$1))

PASS_ASSEMBLE := $(call get_obj_name,$(wildcard pass_compile/*))

clean_FILES += *.to *.texe
clean_FILES += ops/*.texe

all: positive
positive: $(PASS_ASSEMBLE)

