TOP := $(abspath ../..)

TOOLDIR := $(shell $(MAKE) -f $(TOP)/Makefile showbuilddir)
TAS = $(TOOLDIR)/tas
TLD = $(TOOLDIR)/tld

INCLUDES += -I../../lib
CPPFLAGS += $(INCLUDES)

vpath %.texe $(TOP)/ex
vpath %.tas.cpp $(TOP)/lib

# Specific rules
trampoline.memh: irq.texe
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

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

