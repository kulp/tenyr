TAS   = ../../tas
TLD   = ../../tld
SHELL = /bin/bash
FLCLI = flcli
SETUP = source ~/ise.sh

DESIGN       = Tenyr
LOADSRC     ?= bm_fib
DEVICE       = xc6slx16-csg324-3
FILTER       = -filter iseconfig/filter.filter
INTSTYLE     = -intstyle silent # or xflow, or ise
CONSTRAINTS  = Tenyr.ucf Nexys3_Master.ucf
EFFORT       = -ol std
THREADING    = -mt 2

.DEFAULT_GOAL = bit
.PHONY: xst ngdbuild map par trce bit xsvf
.PRECIOUS: %.xsvf %.bit %.ngc %.ngd %.ncd

xst:      $(DESIGN).ngc
ngdbuild: $(DESIGN).ngd
map:      $(DESIGN)_map.ncd
par:      $(DESIGN).ncd
trce:     $(DESIGN).twr
bit:      $(DESIGN).bit
xsvf:     $(DESIGN).xsvf

burn: $(DESIGN).xsvf
	$(FLCLI) -v 1443:0007 -i 1443:0007 -x $<

vpath %.tas     ../verilog ../../ex
vpath %.tas.cpp ../verilog ../../ex
vpath %.texe    ../verilog ../../ex

ipcore_dir/%.v: ipcore_dir/gen_%.tcl ipcore_dir/%.xco
	(cd $(<D) ; xtclsh $(<F))

Tenyr.ngc: ../verilog/common.vh
Tenyr.ngc: gen/$(LOADSRC).memh
Tenyr.ngd Tenyr.twr: $(CONSTRAINTS)
-include Tenyr.md

################################################################################
# General rules
%.md: %.prj
	perl -nae 'BEGIN{print "$*.ngc: "}$$_=$$F[2]; y/"//d; print "$$_ "' $< > $@.$$$$ && mv $@.$$$$ $@ || rm $@.$$$$

# force remaking %.texe because we don't have actual dependency information here
FORCE:
%.texe: %.tas FORCE
	@$(MAKE) -C $(<D) $@
	@cp -p $(<D)/$(@F) $@

%.texe: %.tas.cpp FORCE
	@$(MAKE) -C $(<D) $@
	@cp -p $(<D)/$(@F) $@

gen/%.memh: %.texe
	@mkdir -p $(@D)
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

%.ngc: %.xst
	@ln -sf $(filter %.memh,$^) baremetal.memh
	xst $(INTSTYLE) $(FILTER) -ifn $<

%.ngd: %.ngc
	ngdbuild $(FILTER) $(INTSTYLE) -sd ipcore_dir $(addprefix -uc ,$(filter %.ucf,$^)) $< $@

# -global_opt speed
%_map.ncd: %.ngd
	map -w $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) -logic_opt on -detail -ir off -pr b -lc auto -o $@ $< $*.pcf

%.ncd: %_map.ncd
	par -w $(FILTER) $(INTSTYLE) $(EFFORT) $(THREADING) $< $@ $*.pcf

%.twx %.twr: %.ncd
	trce $(FILTER) $(INTSTYLE) -v 3 -s 3 -n 3 -fastpaths -xml $*.twx $< -o $*.twr $*.pcf $(addprefix -ucf ,$(filter %.ucf,$^))

%.bit: %.ncd
	bitgen -w $(FILTER) $(INTSTYLE) $< $@ $*.pcf

%.xsvf: %.bit burn.batch.tmpl
	sed -e 's/{TOP_LEVEL}/$*/g' burn.batch.tmpl > $@.batch.$$$$ && \
	$(SETUP) && impact -batch $@.batch.$$$$ ;\
	rm $@.batch.$$$$

clean:
	@echo 'Run `make clobber` to clean up generated files'

clobber:
	xargs rm -rf < .gitignore
	git ls-files --others | xargs rm -rf

