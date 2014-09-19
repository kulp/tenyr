TOP = $(abspath ../..)
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

.DEFAULT_GOAL = bit

TAS = ./bin/tas
TLD = ./bin/tld

export TAS
export TLD

$(TAS) $(TLD): $(TOP)/Makefile
	mkdir -p $(@D)
	$(MAKE) BUILDDIR=$(@D) -f $< $(@F)

ifeq ($(XILINX),)
$(error Set XILINX to /path/to/Xilinx/<VERSION>/ISE_DS/ISE)
endif

ARCH := lin$(shell uname -m | grep -o 64)
BIN = $(XILINX)/bin/$(ARCH)

DESIGN       = Tenyr
LOADSRC     ?= clock
DEFAULTSRC   = clock
DEVICE       = xc6slx16-csg324-3
FILTER       = -filter filter.filter
INTSTYLE     = -intstyle silent # or xflow, or ise
CONSTRAINTS  = Tenyr.ucf Nexys3_Master.ucf
EFFORT       = -ol std
THREADING    = -mt 2
XST_TMPDIR   = xst/projnav.tmp

ifeq ($V,1)
INTSTYLE = -intstyle xflow
endif

Tenyr.ngc: ../verilog/common.vh
Tenyr.ngd Tenyr.twr: $(CONSTRAINTS)
include Tenyr.md

################################################################################
# General rules
vpath %.tas     ../verilog $(TOP)/ex
vpath %.tas.cpp ../verilog $(TOP)/ex
vpath %.texe    ../verilog $(TOP)/ex
vpath %.memh    gen

.PHONY: xst ngdbuild map par trce bit xsvf burn
.PRECIOUS: %.xsvf %.bit %.ngc %.ngd %.ncd
.SECONDARY: $(DESIGN)_bd.bmm
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

%.md: %.prj
	perl -nae 'BEGIN{print "$*.ngc: "}$$_=$$F[2]; y/"//d; print "$$_ "' $< > $@.$$$$ && mv $@.$$$$ $@ || rm $@.$$$$

# force remaking %.texe because we don't have actual dependency information here
FORCE:
%.texe: %.tas FORCE | $(TAS) $(TLD)
	@$(MAKE) -C $(<D) $@ TAS=$(PWD)/$(TAS) TLD=$(PWD)/$(TLD)
	@cp -p $(<D)/$(@F) $@

%.texe: %.tas.cpp FORCE | $(TAS) $(TLD)
	@$(MAKE) -C $(<D) $@ TAS=$(PWD)/$(TAS) TLD=$(PWD)/$(TLD)
	@cp -p $(<D)/$(@F) $@

gen/%.memh: %.texe | $(TAS)
	@mkdir -p $(@D)
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

gen/%.mem: gen/%.memh
	# TODO document / hoist start address
	echo '@1000' > $@.$$$$ && cat $< >> $@.$$$$ && mv $@.$$$$ $@ || rm $@.$$$$

$(XST_TMPDIR):
	mkdir -p $@

# Don't automatically regenerate baremetal.memh (use .bmm rules to insert code)
%.ngc: %.xst | baremetal.memh $(XST_TMPDIR)
	$(BIN)/xst $(INTSTYLE) $(FILTER) -ifn $<

%.ngd: %.ngc %.bmm
	$(BIN)/ngdbuild $(FILTER) $(INTSTYLE) -bm $*.bmm -sd ipcore_dir $(addprefix -uc ,$(filter %.ucf,$^)) $< $@

# -global_opt speed
%_map.ncd: %.ngd
	$(BIN)/map -w $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) -logic_opt on -detail -ir off -pr b -lc auto -o $@ $< $*.pcf

%.ncd: %_map.ncd
	$(BIN)/par -w $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) $< $@ $*.pcf

%.xdl: %.ncd
	$(BIN)/xdl -ncd2xdl $< $@

%.twx %.twr: %.ncd
	$(BIN)/trce $(FILTER) $(INTSTYLE) -v 3 -s 3 -n 3 -fastpaths -xml $*.twx $< -o $*.twr $*.pcf $(addprefix -ucf ,$(filter %.ucf,$^))

%.bit %_bd.bmm: %.ncd
	$(BIN)/bitgen -w $(FILTER) $(INTSTYLE) $< $*.bit $*.pcf

%_$(LOADSRC).bit: %.bit gen/$(LOADSRC).mem %_bd.bmm
	$(BIN)/data2mem -bm $(filter %.bmm,$^) -bd $(filter %.mem,$^) -bt $< -o b $@

%.impact.in: %.bit
	@echo > $@.$$$$ "setMode -bs"                       && \
	echo >> $@.$$$$ "setCable -port xsvf -file $*.xsvf" && \
	echo >> $@.$$$$ "addDevice -p 1 -file $<"           && \
	echo >> $@.$$$$ "program -p 1"                      && \
	echo >> $@.$$$$ "quit"                              && \
	mv $@.$$$$ $@ || rm $@.$$$$

%.xsvf: %.impact.in %.bit
	$(BIN)/impact -batch $<

clean_FILES =
clobber_FILES := $(shell cat .gitignore) $($(GIT) ls-files --others) bin/ \
                 $(shell find . -type d -empty | sort -r | xargs rmdir)