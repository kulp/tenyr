.DEFAULT_GOAL = bit

TAS = ./bin/tas
TLD = ./bin/tld

export TAS
export TLD

$(TAS) $(TLD): ../../Makefile
	mkdir -p $(@D)
	$(MAKE) BUILDDIR=$(@D) -f $< $(@F)

ifeq ($(XILINX),)
$(error Set XILINX to /path/to/Xilinx/<VERSION>/ISE_DS/ISE)
endif

ARCH := lin$(shell uname -m | grep -o 64)
BIN = $(XILINX)/bin/$(ARCH)

DESIGN       = Tenyr
LOADSRC     ?= clock
DEVICE       = xc6slx16-csg324-3
FILTER       = -filter iseconfig/filter.filter
INTSTYLE     = -intstyle silent # or xflow, or ise
CONSTRAINTS  = Tenyr.ucf Nexys3_Master.ucf
EFFORT       = -ol std
THREADING    = -mt 2

ifeq ($V,1)
INTSTYLE = -intstyle xflow
endif

Tenyr.ngc: ../verilog/common.vh
Tenyr.ngd Tenyr.twr: $(CONSTRAINTS)
-include Tenyr.md

################################################################################
# General rules
vpath %.tas     ../verilog ../../ex
vpath %.tas.cpp ../verilog ../../ex
vpath %.texe    ../verilog ../../ex
vpath %.memh    gen

# We would like to also regenerate the .xco if the .tcl file changes, but it
# doesn't seem possible to express the necessary circular rule.
ipcore_dir/%.v: ipcore_dir/gen_%.tcl ipcore_dir/%.xco
	(cd $(@D) ; $(BIN)/xtclsh $(notdir $(filter %.tcl,$^)))
	sed -i.bak -e '/^# \(Date\|CRC\):/s/:.*/: .../' $(filter %.xco,$^)
	touch -r $(filter %.xco,$^) $@ # ensure sed doesn't make .xco too new
	-rm $(filter %.xco,$^).bak

.PHONY: xst ngdbuild map par trce bit xsvf burn
.PRECIOUS: %.xsvf %.bit %.ngc %.ngd %.ncd
xst:      $(DESIGN).ngc
ngdbuild: $(DESIGN).ngd
map:      $(DESIGN)_map.ncd
par:      $(DESIGN).ncd
trce:     $(DESIGN).twr
bit:      $(DESIGN).bit
xsvf:     $(DESIGN).xsvf

baremetal.memh: gen/$(LOADSRC).memh
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

# Don't automatically regenerate baremetal.memh (use .bmm rules to insert code)
%.ngc: %.xst | baremetal.memh
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

clean:
	@echo 'Run `make clobber` to clean up generated files'

clobber:
	xargs rm -rf < .gitignore
	git ls-files --others | xargs rm -rf
	-rm -f $(TAS) $(TLD)

