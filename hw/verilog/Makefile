TOP := $(abspath ../..)
include $(TOP)/Makefile.common
include $(TOP)/Makefile.rules

INCLUDES += -I../../lib
CPPFLAGS += $(INCLUDES)

vpath %.texe $(TOP)/ex
vpath %.tas.cpp $(TOP)/lib

# Specific rules
trampoline.memh: irq.memh
	cp $< $@

