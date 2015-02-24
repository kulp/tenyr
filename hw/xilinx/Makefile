TOP = $(abspath ../..)
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

.DEFAULT_GOAL = bit

# Optimisations on by default
OPT := 1
# Verbosity off by default
V   :=#

TAS = $(abspath .)/bin/tas
TLD = $(abspath .)/bin/tld

export TAS
export TLD

$(TAS) $(TLD): $(TOP)/Makefile
	mkdir -p $(@D)
	$(MAKE) BUILDDIR=$(@D) -f $< $(@F)

ifeq ($(XILINX),)
$(warning Set XILINX to /path/to/Xilinx/<VERSION>/ISE_DS/ISE)
endif

ARCH := lin$(shell uname -m | grep -o 64)
BIN = $(XILINX)/bin/$(ARCH)/

DESIGN       = Tenyr
LOADSRC     ?= clock
DEFAULTSRC   = clock
DEVICE       = xc6slx16-csg324-3
FILTER       = -filter filter.filter
INTSTYLE     = -intstyle silent # or xflow, or ise
EFFORT       = -ol std
THREADING    = -mt 2
XST_TMPDIR   = xst/projnav.tmp

OPTS_xst      = $(FILTER) $(INTSTYLE)
OPTS_ngdbuild = $(FILTER) $(INTSTYLE)
OPTS_map      = $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) -w
OPTS_par      = $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) -w
OPTS_trce     = $(FILTER) $(INTSTYLE) -v 3 -s 3 -n 3 -fastpaths
OPTS_bitgen   = $(FILTER) $(INTSTYLE) -w

ifeq ($V,1)
INTSTYLE = -intstyle xflow
OPTS_map += -detail
endif

ifeq ($(OPT),1)
OPTS_map += -logic_opt on -pr b -lc auto
endif

ifeq ($(OPT),2)
OPTS_map += -logic_opt on -pr b -lc auto -global_opt speed
EFFORT = -ol high
endif

Tenyr.ngc: ../verilog/common.vh

################################################################################
# General rules
vpath %.tas     ../verilog $(TOP)/ex
vpath %.tas.cpp ../verilog $(TOP)/ex
vpath %.texe    ../verilog $(TOP)/ex
vpath %.memh    gen

.PHONY: xst ngdbuild map par trce bit xsvf burn
.SECONDARY: $(DESIGN).ngc $(DESIGN).ngd $(DESIGN)_map.ncd $(DESIGN).ncd
.SECONDARY: $(DESIGN).bit $(DESIGN)_bd.bmm $(DESIGN).xsvf
xst:      $(DESIGN).ngc
ngdbuild: $(DESIGN).ngd
map:      $(DESIGN)_map.ncd
par:      $(DESIGN).ncd
trce:     $(DESIGN).twr
bit:      $(DESIGN).bit
xsvf:     $(DESIGN).xsvf

baremetal.memh: gen/$(DEFAULTSRC).memh
	cp -p $< $@

ifeq ($(BURNTOOL),djtgcfg)
burn: $(DESIGN)_$(LOADSRC).bit
	djtgcfg prog -d Nexys3 -f $< -i 0
else
burn: $(DESIGN)_$(LOADSRC).xsvf
	flcli --vp=1443:0007 --ivp=1443:0007 --xsvf=$<
endif

include $(DESIGN).md
%.md: %.prj
	perl -nae 'BEGIN{print "$*.ngc: "}$$_=$$F[2]; y/"//d; print "$$_ "' $< > $@

# force remaking %.texe because we don't have actual dependency information here
FORCE:
%.texe: %.tas.cpp FORCE | $(TAS) $(TLD)
	@$(MAKESTEP) -n "Recursively building $@ from $< ... "
	$(SILENCE)$(MAKE) -s -C $(<D) $@ TAS=$(TAS) TLD=$(TLD)
	$(SILENCE)cp -p $(<D)/$(@F) $@
	@$(MAKESTEP) done

%.texe: %.tas FORCE | $(TAS) $(TLD)
	@$(MAKESTEP) -n "Recursively building $@ from $< ... "
	$(SILENCE)$(MAKE) -s -C $(<D) $@ TAS=$(TAS) TLD=$(TLD)
	$(SILENCE)cp -p $(<D)/$(@F) $@
	@$(MAKESTEP) done

gen/%.memh: %.texe | $(TAS)
	$(SILENCE)mkdir -p $(@D)
	@# TODO document / hoist start address
	$(SILENCE)$(TAS) -vd $< | $(TAS) -p format.memh.offset=0x1000 -fmemh -o $@ -

gen/%.mem: gen/%.memh
	cp -p $< $@

$(XST_TMPDIR):
	mkdir -p $@

# Don't automatically regenerate baremetal.memh (use .bmm rules to insert code)
%.ngc: %.xst | baremetal.memh $(XST_TMPDIR)
	$(BIN)xst $(OPTS_xst) -ifn $<

%.ngd: %.ngc %.bmm %.ucf
	$(BIN)ngdbuild $(OPTS_ngdbuild) -bm $*.bmm $< $@

%_map.ncd: %.ngd
	$(BIN)map $(OPTS_map) -o $@ $< $*.pcf

%.ncd: %_map.ncd
	$(BIN)par $(OPTS_par) $< $@ $*.pcf

%.xdl: %.ncd
	$(BIN)xdl -ncd2xdl $< $@

%.twx %.twr: %.ncd %.ucf
	$(BIN)trce $(OPTS_trce) -ucf $*.ucf -xml $*.twx $< -o $*.twr $*.pcf

%.bit %_bd.bmm: %.ncd
	$(BIN)bitgen $(OPTS_bitgen) $< $*.bit $*.pcf

%_$(LOADSRC).bit: gen/$(LOADSRC).mem %_bd.bmm %.bit
	$(BIN)data2mem -bd $< -bm $*_bd.bmm -bt $*.bit -o b $@

%.impact.in: %.bit
	@echo > $@ "setMode -bs"                       && \
	echo >> $@ "setCable -port xsvf -file $*.xsvf" && \
	echo >> $@ "addDevice -p 1 -file $<"           && \
	echo >> $@ "program -p 1"                      && \
	echo >> $@ "quit"

%.xsvf: %.impact.in %.bit
	$(BIN)impact -batch $<

clean_FILES =
clobber_FILES := $(shell cat .gitignore) $($(GIT) ls-files --others) bin/
clobber::
	find . -type d -empty | xargs rmdir -p
