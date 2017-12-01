makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))/..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

SHELL := $(shell which bash)

$(build_tas) $(build_tsim) $(build_tld):
	$(MAKE) -f $(TOP)/Makefile $@

INSTALL_DIR ?= /usr/local

local-install: INSTALL_DIR = $(TOP)/dist/$(MACHINE)
local-install: install

install:: $(BIN_TARGETS)
	install -d $(INSTALL_DIR)/bin
	install $^ $(INSTALL_DIR)/bin

install:: $(LIB_TARGETS)
	install -d $(INSTALL_DIR)/lib
	$(if $(LIB_TARGETS),install $^ $(INSTALL_DIR)/lib)

install:: $(RESOURCES)
	install -d $(subst $(TOP)/,$(INSTALL_DIR)/share/tenyr/,$(^D))
	$(foreach f,$^,install -m 0644 $f $(INSTALL_DIR)/share/tenyr/$(subst $(TOP)/,,$(dir $f));)

uninstall:: $(BIN_TARGETS)
	$(RM) $(foreach t,$(^F),$(INSTALL_DIR)/bin/$t)

uninstall:: $(LIB_TARGETS)
	$(RM) $(foreach t,$(^F),$(INSTALL_DIR)/lib/$t)

uninstall:: $(RESOURCES)
	$(RM) -r $(INSTALL_DIR)/share/tenyr

doc: tas_usage tsim_usage tld_usage

%_usage: %$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Generating usage description for $* ... "
	$($*) --help | \
		sed -e 's/^/    /' \
		    -e '/version/s/-[0-9][0-9]*-g[[:xdigit:]]\{7\}/$1.../' \
	        > $(TOP)/wiki/$*--help.md && $(MAKESTEP) ok

gzip zip: export CREATE_TEMP_INSTALL_DIR=1

ifeq ($(CREATE_TEMP_INSTALL_DIR),1)
gzip: tenyr-$(BUILD_NAME).tar.gz
zip: tenyr-$(BUILD_NAME).zip

ZIPBALLS = tenyr-$(BUILD_NAME).zip tenyr-$(BUILD_NAME).tar.gz
$(ZIPBALLS): INSTALL_PRE := $(shell mktemp -d tenyr.dist.XXXXXX)
$(ZIPBALLS): INSTALL_DIR := $(INSTALL_PRE)/tenyr-$(BUILD_NAME)

tenyr-$(BUILD_NAME).tar.gz: install
	tar zcf $@ -C $(INSTALL_DIR)/.. .
	$(RM) -r $(INSTALL_PRE)

tenyr-$(BUILD_NAME).zip: install
	orig=$(abspath $@) && (cd $(INSTALL_DIR)/.. ; zip -r $$orig .)
	$(RM) -r $(INSTALL_PRE)

else
gzip zip:
	$(MAKE) -f $(makefile_path) $@
endif

.SECONDARY: coverage.info.src coverage.info.vpi
coverage: coverage_html_src_vpi

LCOV ?= lcov

.PHONY: lcov_setup
# LCOV 1.9 doesn't support `--rc` or `--config-file` so it is necessary to copy
# our lcovrc into a home directory. This process will be skipped if an lcovrc
# already exists.
lcov_setup:
	[[ $$CI = "true" ]] && cp $(TOP)/scripts/lcovrc ~/.lcovrc || true

COVERAGE_RULE = check
coverage.info: $(COVERAGE_RULE) | lcov_setup
	$(LCOV) --capture --test-name $< --directory $(BUILDDIR) --output-file $@

COVERAGE_SKIP = 3rdparty/asmjit/*
coverage.info.trimmed: coverage.info
	$(LCOV) --output-file $@ $(foreach f,$(COVERAGE_SKIP),--remove $< '$f')

coverage.info.%: coverage.info.trimmed
	$(LCOV) --extract $< '*/$*/*' --output-file $@

coverage_html_src:     coverage.info.src
coverage_html_src_vpi: coverage.info.src coverage.info.vpi
coverage_html_%:
	genhtml --output-directory $@ $^

check: check_sw check_hw
check_sw: check_args check_behaviour check_compile check_sim check_obj check_forth dogfood
check_forth:
	@$(MAKESTEP) -n "Compiling forth ... "
	$(MAKE) $S BUILDDIR=$(abspath $(BUILDDIR)) -C $(TOP)/forth && $(MAKESTEP) ok

check_args: check_args_tas check_args_tld check_args_tsim
check_args_%: check_args_general_% check_args_specific_% ;

check_args_general_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* general options ... "
	$($*) -V | grep -q version                                   && $(MAKESTEP) "    ... -V ok"
	$($*) -h | grep -q Usage                                     && $(MAKESTEP) "    ... -h ok"
	( ! $($*) ) > /dev/null 2>&1                                 && $(MAKESTEP) "    ... no-args trapped ok"
	$($*) /dev/non-existent-file 2>&1 | grep -q "Failed to open" && $(MAKESTEP) "    ... non-existent file ok"
	$($*) -QRSTU 2>&1 | egrep -qi "(Invalid|unknown|illegal) op" && $(MAKESTEP) "    ... bad option prints error ok"
	( ! $($*) -QRSTU &> /dev/null )                              && $(MAKESTEP) "    ... bad option exits non-zero ok"

check_args_specific_%: %$(EXE_SUFFIX) ;

check_args_specific_tas: check_args_specific_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* specific options ... "
	echo -n 9876 | $($*) -fraw -d - | fgrep -q ".word 0x3637"    && $(MAKESTEP) "    ... -d ok"
	(! $($*) -f does_not_exist /dev/null &> /dev/null )          && $(MAKESTEP) "    ... -f ok"
	echo -n 9876 | $($*) -fraw -d -q - | fgrep -qv "word 0x3637" && $(MAKESTEP) "    ... -q ok"
	$($*) -d -ftext -v - <<<0xc | fgrep -q "A + 0x0000000c"      && $(MAKESTEP) "    ... -v ok"
	echo '.zero 2' | $($*) -fmemh -pformat.memh.explicit=1 - | fgrep -q "@0 00000000" \
	                                                             && $(MAKESTEP) "    ... memh explicit ok"
	echo '.word 1' | $($*) -fmemh -pformat.memh.offset=5 -   | fgrep -q "@5 00000001" \
	                                                             && $(MAKESTEP) "    ... memh offset ok"
	echo '.word 1' | $($*) -fmemh -p{A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z}=1 \
	                              -pformat.memh.offset=5 -   | fgrep -q "@5 00000001" \
	                                                             && $(MAKESTEP) "    ... params overflow ok"

check_args_specific_tsim: s = 4105
check_args_specific_tsim: sx = $(shell printf 0x%08x $(s))
check_args_specific_tsim: check_args_specific_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* specific options ... "
	$($*) -@ does_not_exist 2>&1 | fgrep -q "No such"            && $(MAKESTEP) "    ... -@ ok"
	$($*) -fraw -a 123 /dev/zero 2>&1 | fgrep -q "address 0x7b"  && $(MAKESTEP) "    ... -a ok"
	$($*) -d -ftext /dev/null 2>&1 | fgrep -q "executed: 1"      && $(MAKESTEP) "    ... -d ok"
	(! $($*) -f does_not_exist /dev/null &> /dev/null )          && $(MAKESTEP) "    ... -f ok"
	(! $($*) -r does_not_exist -fraw /dev/null &> /dev/null )    && $(MAKESTEP) "    ... -r ok"
	$($*) -fraw  -vs $(s) /dev/null 2>&1 | fgrep -q "IP = $(sx)" && $(MAKESTEP) "    ... -s ok"
	$($*) -ftext -v       /dev/null 2>&1 | fgrep -q "IP ="       && $(MAKESTEP) "    ... -v ok"
	$($*) -ftext -vv      /dev/null 2>&1 | fgrep -q ".word"      && $(MAKESTEP) "    ... -vv ok"
	$($*) -ftext -vvv     /dev/null 2>&1 | fgrep -q "read  @"    && $(MAKESTEP) "    ... -vvv ok"
	$($*) -ftext -vvvv    /dev/null 2>&1 | fgrep -q "P 00001"    && $(MAKESTEP) "    ... -vvvv ok"
	$($*) -ftext bad0 bad1 2>&1 | fgrep -qi "more than one"      && $(MAKESTEP) "    ... multiple files rejected ok"
	$($*) -d -ftext - < /dev/null 2>&1 | fgrep -q "executed: 1"  && $(MAKESTEP) "    ... stdin accepted for input ok"
	$(if $(findstring emscripten,$(PLATFORM)),,(! $($*) -remscript - &> /dev/null )  && $(MAKESTEP) "    ... emscripten recipe rejected ok")

check_args_specific_tld: check_args_specific_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* specific options ... "
	$($*) - < $(TOP)/test/misc/obj/empty.to 2>&1 | fgrep -q "TOV"  && $(MAKESTEP) "    ... stdin accepted for input ok"

check_behaviour: check_behaviour_tas check_behaviour_tld check_behaviour_tsim
check_behaviour_%: ;

check_behaviour_tas: MEMHD = $(TOP)/test/misc/memh/
check_behaviour_tas: OBJD = $(TOP)/test/misc/obj/
check_behaviour_tas: TASD = $(TOP)/test/misc/
check_behaviour_tas: check_behaviour_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* behaviour ... "
	$($*) -o . /dev/null 2>&1 | fgrep -qi "failed to open"                     && $(MAKESTEP) "    ... failed to open ok"
	(! $($*) -d -f memh $(MEMHD)backward.memh &>/dev/null )                    && $(MAKESTEP) "    ... validated memh lack of backward support ok"
	$($*) -d $(OBJD)bad_version.to 2>&1 | fgrep -qi "unhandled version"        && $(MAKESTEP) "    ... unhandled version ok"
	$($*) -d $(OBJD)toolarge.to 2>&1 | fgrep -q "too large"                    && $(MAKESTEP) "    ... too-large ok"
	$($*) -d $(OBJD)toolarge2.to 2>&1 | fgrep -q "too large"                   && $(MAKESTEP) "    ... too-large 2 ok"
	$($*) $(TASD)missing_global.tas 2>&1 | fgrep -q "not defined"              && $(MAKESTEP) "    ... undefined global ok"

check_behaviour_tld: OBJD = $(TOP)/test/misc/obj/
check_behaviour_tld: check_behaviour_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* behaviour ... "
	$($*) /dev/null 2>&1 | fgrep -qi "end of file"                              && $(MAKESTEP) "    ... too-small ok"
	$($*) $(OBJD)toolarge.to 2>&1 | fgrep -q "too large"                        && $(MAKESTEP) "    ... too-large ok"
	$($*) -o $(OBJD) /dev/null 2>&1 | fgrep -qi "failed to open"                && $(MAKESTEP) "    ... failed to open ok"
	$($*) $(OBJD)duplicate.to $(OBJD)duplicate.to 2>&1 | fgrep -qi "duplicate"  && $(MAKESTEP) "    ... duplicate symbols ok"
	$($*) $(OBJD)zerorecs.to 2>&1 | fgrep -qi "has no records"                  && $(MAKESTEP) "    ... zero records ok"
	$($*) $(OBJD)tworecs.to 2>&1 | fgrep -qi "more than one record"             && $(MAKESTEP) "    ... multiple records ok"
	$($*) $(OBJD)unresolved.to 2>&1 | fgrep -qi "missing definition"            && $(MAKESTEP) "    ... unresolved ok"

clean_FILES += check_obj_*.to null.to ff.bin
null.to: ; $(tas) -o $@ /dev/null
ff.bin:; echo -ne '\0377\0377\0377\0377' > $@
check_obj: check_obj_0 check_obj_2 check_obj_4 check_obj_5 check_obj_6
check_obj_%: check_obj_%.to | $(build_tas)
	(! $(tas) -d $< 2> /dev/null)
check_obj_%.to: null.to ff.bin
	cp $< $@
	dd bs=4 if=ff.bin of=$@ seek=$* 2>/dev/null
	dd bs=4 if=$< of=$@ skip=$$(($*+1)) seek=$$(($*+1)) 2>/dev/null

vpath %_demo.tas.cpp $(TOP)/ex
DEMOS = qsort bsearch trailz
DEMOFILES = $(DEMOS:%=$(TOP)/ex/%_demo.texe)
OPS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/op/*.tas)))
OPS += $(subst .tas.cpp,,$(notdir $(wildcard $(TOP)/test/op/*.tas.cpp)))
RUNS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/run/*.tas)))
#TESTS = $(patsubst $(TOP)/test/%.tas,%,$(wildcard $(TOP)/test/*/*.tas))
$(DEMOFILES): %_demo.texe: %_demo.tas.cpp
	@$(MAKESTEP) -n "Building $(*F) demo ... "
	$(MAKE) $S BUILDDIR=$(abspath $(BUILDDIR)) -C $(@D) $(@F) && $(MAKESTEP) ok

# This test needs an additional object, as it tests the linker
$(TOP)/test/run/reloc_set.texe: $(TOP)/test/misc/reloc_set0.to
# This test needs imul compiled in
$(TOP)/test/run/test_imul.texe: $(TOP)/lib/imul.to
$(TOP)/test/run/reloc_shifts.texe: $(TOP)/test/misc/reloc_shifts0.to

check_hw:
	@$(MAKESTEP) -n "Checking for Icarus Verilog ... "
	if $(TOP)/scripts/check_icarus.sh "`which $(IVERILOG)iverilog`" ; then \
		$(MAKE) -C $(BUILDDIR) -f $(TOP)/Makefile vpi ; \
		$(MAKE) -C $(BUILDDIR) -f $(makefile_path) BUILDDIR=$(BUILDDIR) \
		    check_hw_icarus_op check_hw_icarus_demo check_hw_icarus_run; \
	fi

test_demo_% test_run_% test_op_%: texe=$<

# Demos are not self-testing, so they have demo-specific external checkers
test_demo_qsort:   verify = sed -n 5p | tr -d '\015'
test_demo_qsort:   result = eight
test_demo_bsearch: verify = grep -v "not found" | wc -l | tr -d ' '
test_demo_bsearch: result = 11
test_demo_trailz:  verify = grep -c good
test_demo_trailz:  result = 33
test_demo_%: record = 2> log.$@.err.$$$$ | tee log.$@.out.$$$$
test_demo_%: $(TOP)/ex/%_demo.texe
	@$(MAKESTEP) -n "Running $* demo ($(context)) ... "
	[ "$$(($(call run,$*)) $(record) | $(verify))" = "$(result)" ] && (rm -f log.$@.{out,err}.$$$$ ; $(MAKESTEP) ok) || (cat log.$@.{out,err}.$$$$ ; false)

randwords = $(shell LC_ALL=C tr -dc "[:xdigit:]" < /dev/urandom | dd conv=lcase | fold -w8 | head -n$1 | sed 's/^/.word 0x/;s/$$/;/')

# Op tests are self-testing -- they must leave B with the value 0xffffffff if successful.
$(OPS:%=$(TOP)/test/op/%.texe): INCLUDES += $(TOP)/test/op
$(OPS:%=$(TOP)/test/op/%.texe): $(TOP)/test/op/%.texe: $(TOP)/test/op/%.to $(TOP)/test/op/args.to | $(build_tas)

# Use .INTERMEDIATE to cause op args files to be deleted after one run
.INTERMEDIATE: $(TOP)/test/op/args.to
$(TOP)/test/op/args.to: | $(build_tas)
	echo ".global args ; args: $(call randwords,3)" | $(tas) -o $@ -

test_op_%: $(TOP)/test/op/%.texe $(build_tas)
	@$(MAKESTEP) -n "Testing op `printf %-7s "'$*'"` ($(context)) ... "
	$(run) && $(MAKESTEP) ok

# Run tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .SECONDARY to indicate that run test files should *not* be deleted after
# one run, as they do not have random bits appended (yet).
.SECONDARY: $(RUNS:%=$(TOP)/test/run/%.texe)
test_run_%: %.texe $(build_tas) $(build_tld)
	@$(MAKESTEP) -n "Running test `printf %-15s "'$*'"` ($(context)) ... "
	$(run) && $(MAKESTEP) ok

check_hw_icarus_pre:
	@$(MAKESTEP) -n "Building hardware simulator ... "
	$(MAKE) $S -C $(TOP)/hw/icarus tenyr && $(MAKESTEP) ok

tsim_FLAVOURS := interp interp_prealloc
tsim_FLAGS_interp =
tsim_FLAGS_interp_prealloc = --scratch --recipe=prealloc --recipe=serial --recipe=plugin
ifneq ($(JIT),0)
tsim_FLAVOURS += jit
tsim_FLAGS_jit = -rjit -ptsim.jit.run_count_threshold=2
check_sim::
	$(MAKE) -f $(TOP)/Makefile libtenyrjit$(DYLIB_SUFFIX)
endif

check_sim check_sim_demo check_sim_op check_sim_run: export context=sim,$(flavour)
check_sim_demo check_sim_op check_sim_run: $(build_tsim)
check_sim::
	$(foreach f,$(tsim_FLAVOURS),$(MAKE) -f $(makefile_path) check_sim_flavour flavour=$f tsim_FLAGS='$(tsim_FLAGS) $(tsim_FLAGS_$f)' &&) true
check_sim_flavour: check_sim_demo check_sim_op check_sim_run

check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: export context=hw_icarus
check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: check_hw_icarus_pre PERIODS.mk

check_hw_icarus_demo: export run=$(MAKE) --no-print-directory -s -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile BUILDDIR=$(abspath $(BUILDDIR)) run_$*_demo | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD
check_sim_demo: export run=$(tsim) $(tsim_FLAGS) $(TOP)/ex/$*_demo.texe

# "SDL-related" tests
# These tests are really device tests that for now require SDL. Since we don't
# always have SDL available, we don't want to run the tests if we are asked not
# to. Hardware tests under Icarus, however, can always run these tests, at
# least until we start hooking up the SDL devices via VPI.
SDL_RUNS = led bm_mults
vpath %.tas  $(TOP)/test/run/sdl
vpath %.texe $(TOP)/test/run/sdl

test_run_led: $(TOP)/test/run/sdl/led.texe
test_run_bm_mults: $(TOP)/ex/bm_mults.texe
# For now we need a special rule to make examples inside their directory to
# pick up dependency information from ex/Makefile.
$(TOP)/ex/%.texe: ; $(MAKE) -C $(@D) $(@F)
$(TOP)/test/run/sdl/%.texe: ; $(MAKE) -C $(TOP)/test run/sdl/$(@F)

export SDL_VIDEODRIVER=dummy
PERIODS.mk: $(SDL_RUNS:%=PERIODS_%.mk)
check_hw_icarus_run: $(SDL_RUNS:%=test_run_%)

ifneq ($(SDL),0)
RUNS += $(SDL_RUNS)
tsim_FLAGS += -p paths.share=$(call os_path,$(TOP))
tsim_FLAGS += -@ $(TOP)/plugins/sdl.rcp
endif

check_sim_demo check_hw_icarus_demo: $(DEMOS:%=test_demo_%)
check_sim_op   check_hw_icarus_op:   $(OPS:%=  test_op_%  )
check_sim_run  check_hw_icarus_run:  $(RUNS:%= test_run_% )

# TODO make op tests take a fixed or predictable maximum amount of time
# The number of cycles necessary to run a code in Verilog simulation is
# determined by running it in tsim and multiplying the number of instructions
# executed by the number of cycles per instruction (currently 10). A margin of
# 2x is added to allow testcases not always to take exactly the same number of
# instructions to complete.
vpath %.texe $(TOP)/test/op $(TOP)/ex $(TOP)/test/run
clean_FILES += $(BUILDDIR)/PERIODS_*.mk
PERIODS_%.mk: %.texe $(build_tsim)
	@$(MAKESTEP) -n "Computing cycle count for '$*' ... "
	$(ECHO) -n PERIODS_$*= > $@
	echo $$(($$($(tsim) -d $< 2>&1 | sed -En '/^.*executed: ([0-9]+)/{s//\1/;p;}') * 20)) >> $@
	cp -f $(TOP)/mk/$(@F) $@ 2>/dev/null || true # override with forced version if existing
	@$(MAKESTEP) ok

vpath PERIODS_%.mk $(TOP)/mk
PERIODS.mk: $(patsubst %,PERIODS_%.mk,$(OPS) $(DEMOS:%=%_demo) $(RUNS))
	cat $^ > $@

check_sim_op check_sim_run: export run=$(tsim) $(tsim_FLAGS) -vvvv $(texe) 2>&1 | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'
check_hw_icarus_op: PERIODS.mk
check_hw_icarus_op check_hw_icarus_run: export run=$(MAKE) -s --no-print-directory -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile run_$* VPATH=$(TOP)/test/op:$(TOP)/test/run BUILDDIR=$(abspath $(BUILDDIR)) PLUSARGS_EXTRA=+DUMPENDSTATE | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'

check_compile: $(build_tas) $(build_tld)
	@$(MAKESTEP) -n "Building tests from test/ ... "
	$(MAKE) $S -C $(TOP)/test && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Building examples from ex/ ... "
	$(MAKE) $S -C $(TOP)/ex && $(MAKESTEP) ok

dogfood: $(wildcard $(TOP)/test/pass_compile/*.tas $(TOP)/ex/*.tas*) $(build_tas)
	@$(MAKESTEP) -n "Checking reversibility of assembly-disassembly ... "
	$(TOP)/scripts/dogfood.sh dogfood.$$$$.XXXXXX "$(tas)" $(filter-out $(build_tas),$^) && $(MAKESTEP) ok || ($(MAKESTEP) FAILED ; false)

