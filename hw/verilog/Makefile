TOP ?= ../..
TAS = $(TOP)/tas
TLD = $(TOP)/tld

vpath %.texe $(TOP)/ex

%.vi: %.texe
	$(TAS) -vd $< | $(TAS) -fverilog -o $@ -

%.memh: %.texe
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

