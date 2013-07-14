TOP ?= ../..
TAS = $(TOP)/tas
TLD = $(TOP)/tld

vpath %.texe $(TOP)/ex

%.memh: %.texe
	$(MAKE) -C $(dir $<) $(notdir $<)
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

