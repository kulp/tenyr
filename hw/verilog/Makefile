TOP ?= ../..
TAS = $(TOP)/tas
TLD = $(TOP)/tld

vpath %.texe $(TOP)/ex

%.vi: %.texe
	$(MAKE) -C $(dir $<) $(notdir $<)
	$(TAS) -vd $< | $(TAS) -fverilog -o $@ -

%.memh: %.texe
	$(MAKE) -C $(dir $<) $(notdir $<)
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

