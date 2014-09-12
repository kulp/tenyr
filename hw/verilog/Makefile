TOP := $(abspath ../..)
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

INCLUDES += -I$(TOP)/lib
CPPFLAGS += $(INCLUDES)

vpath %.texe $(TOP)/ex
vpath %.tas.cpp $(TOP)/lib

# Specific rules
trampoline.memh: irq.memh
	cp $< $@

