TOP ?= ../..
TAS = $(TOP)/tas
TLD = $(TOP)/tld

INCLUDES += -I../../lib
CPPFLAGS += -xc $(INCLUDES)
CPP := $(CPP) -

vpath %.texe $(TOP)/ex
vpath %.tas.cpp $(TOP)/lib

# Specific rules
trampoline.memh: irq.texe
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

# Pattern rules
%.vi: %.texe
	$(MAKE) -C $(dir $<) $(notdir $<)
	$(TAS) -vd $< | $(TAS) -fverilog -o $@ -

%.memh: %.texe
	$(MAKE) -C $(dir $<) $(notdir $<)
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

%.tas: %.tas.cpp
	mkdir -p $(*D)
	$(CPP) $(CPPFLAGS) - < $< > $@

%.to: %.tas
	mkdir -p $(*D)
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

