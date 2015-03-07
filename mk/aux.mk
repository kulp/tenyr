makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))/..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

SHELL := $(shell which bash)

tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX):
	$(SILENCE)$(MAKE) -s -f $(TOP)/Makefile $@

INSTALL_DIR ?= /usr/local

local-install: INSTALL_DIR = $(TOP)/dist/$(MACHINE)
local-install: install

install:: $(BIN_TARGETS)
	install -d $(INSTALL_DIR)/bin
	install $^ $(INSTALL_DIR)/bin

install:: $(LIB_TARGETS)
	install -d $(INSTALL_DIR)/lib
	install $^ $(INSTALL_DIR)/lib

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
	$(SILENCE)PATH=$(BUILDDIR) $< --help | \
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

coverage: coverage_html_src

coverage.info: check_sw
	lcov --capture --test-name $< --directory $(BUILDDIR) --output-file $@

COVERAGE_SKIP = spi.c
coverage.info.trimmed: coverage.info
	lcov --output-file $@ $(foreach f,$(COVERAGE_SKIP),--remove $< '*/$f')

coverage.info.%: coverage.info.trimmed
	lcov --extract $< '*/$*/*' --output-file $@

coverage_html_%: coverage.info.%
	genhtml $< --output-directory $@

check: check_sw check_hw
check_sw: check_args check_compile check_sim check_forth dogfood
check_forth:
	@$(MAKESTEP) -n "Compiling forth ... "
	$(SILENCE)$(MAKE) $S BUILDDIR=$(abspath $(BUILDDIR)) -C $(TOP)/forth && $(MAKESTEP) ok

check_args: check_args_tas check_args_tld check_args_tsim
check_args_%: check_args_general_% check_args_specific_% ;

check_args_general_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* general options ... "
	$(SILENCE)$(BUILDDIR)/$< -V | grep -q version                                   && $(ECHO) "    ... -V ok"
	$(SILENCE)$(BUILDDIR)/$< -h | grep -q Usage                                     && $(ECHO) "    ... -h ok"
	$(SILENCE)( ! $(BUILDDIR)/$< ) > /dev/null 2>&1                                 && $(ECHO) "    ... no-args trapped ok"
	$(SILENCE)$(BUILDDIR)/$< /dev/non-existent-file 2>&1 | grep -q "Failed to open" && $(ECHO) "    ... non-existent file ok"
	$(SILENCE)$(BUILDDIR)/$< -QRSTU 2>&1 | grep -qi "Invalid option"                && $(ECHO) "    ... bad option prints error ok"
	$(SILENCE)( ! $(BUILDDIR)/$< -QRSTU &> /dev/null )                              && $(ECHO) "    ... bad option exits non-zero ok"

check_args_specific_%: %$(EXE_SUFFIX) ;

check_args_specific_tas: check_args_specific_%: %$(EXE_SUFFIX)
	@$(MAKESTEP) "Checking $* specific options ... "
	$(SILENCE)$(BUILDDIR)/$< -f invalid /dev/null | grep -q Usage                   && $(ECHO) "    ... -f invalid ok"
	$(SILENCE)$(BUILDDIR)/$< -d -ftext - <<<0xc -v | fgrep -q "A + 0x0000000c"      && $(ECHO) "    ... -v ok"

vpath %_demo.tas.cpp $(TOP)/ex
DEMOS = qsort bsearch trailz
DEMOFILES = $(DEMOS:%=$(TOP)/ex/%_demo.texe)
OPS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/op/*.tas)))
RUNS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/run/*.tas)))
#TESTS = $(patsubst $(TOP)/test/%.tas,%,$(wildcard $(TOP)/test/*/*.tas))
$(DEMOFILES): %_demo.texe: %_demo.tas.cpp
	@$(MAKESTEP) -n "Building $(*F) demo ... "
	$(SILENCE)$(MAKE) $S BUILDDIR=$(abspath $(BUILDDIR)) -C $(@D) $(@F) && $(MAKESTEP) ok

check_hw:
	@$(MAKESTEP) -n "Checking for Icarus Verilog ... "
	$(SILENCE)icarus="$$($(MAKE) --no-print-directory $S -i -C $(TOP)/hw/icarus has-icarus)" ; \
	if [ -x "$$icarus" ] ; then \
		$(MAKESTEP) $$icarus ; \
		$(MAKE) -C $(BUILDDIR) -f $(makefile_path) BUILDDIR=$(BUILDDIR) \
		    check_hw_icarus_op check_hw_icarus_demo check_hw_icarus_run; \
	else \
		$(MAKESTEP) "not found" ; \
	fi

# Demos are not self-testing, so they have demo-specific external checkers
test_demo_qsort:   verify = sed -n 5p
test_demo_qsort:   result = eight
test_demo_bsearch: verify = grep -v "not found" | wc -l | tr -d ' '
test_demo_bsearch: result = 11
test_demo_trailz:  verify = grep -c good
test_demo_trailz:  result = 33
test_demo_%: $(TOP)/ex/%_demo.texe
	@$(MAKESTEP) -n "Running $* demo ($(context)) ... "
	$(SILENCE)[ "$$($(call run,$*) | $(verify))" = "$(result)" ] && $(MAKESTEP) ok

randwords = $(shell LC_ALL=C tr -dc "[:xdigit:]" < /dev/urandom | dd conv=lcase | fold -w8 | head -n$1 | sed 's/^/.word 0x/;s/$$/;/')

# Op tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .INTERMEDIATE to indicate that op test files should be deleted after one
# run -- they have random bits appended which should be regenerated each time.
.INTERMEDIATE: $(OPS:%=$(TOP)/test/op/%.texe)
$(TOP)/test/op/%.texe: $(TOP)/test/op/%.tas tas$(EXE_SUFFIX)
	$(SILENCE)(cat $< ; echo "$(call randwords,3)") | $(BUILDDIR)/tas -o $@ -

test_op_%: $(TOP)/test/op/%.texe tas$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Testing op `printf %-7s "'$*'"` ($(context)) ... "
	$(SILENCE)$(run) && $(MAKESTEP) ok

# Run tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .SECONDARY to indicate that run test files should *not* be deleted after
# one run, as they do not have random bits appended (yet).
.SECONDARY: $(RUNS:%=$(TOP)/test/run/%.texe)
test_run_%: $(TOP)/test/run/%.texe tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Running test `printf %-12s "'$*'"` ($(context)) ... "
	$(SILENCE)$(run) && $(MAKESTEP) ok

check_hw_icarus_pre:
	@$(MAKESTEP) -n "Building hardware simulator ... "
	$(SILENCE)$(MAKE) $S -C $(TOP)/hw/icarus tenyr && $(MAKESTEP) ok

tsim_FLAVOURS := interp
tsim_FLAGS_interp =
ifneq ($(JIT),0)
tsim_FLAVOURS += jit
tsim_FLAGS_jit = -rjit
endif

check_sim check_sim_demo check_sim_op check_sim_run: export context=sim,$(flavour)
check_sim_demo check_sim_op check_sim_run: tsim$(EXE_SUFFIX)
check_sim:
	$(foreach f,$(tsim_FLAVOURS),$(MAKE) -f $(makefile_path) check_sim_flavour flavour=$f tsim_FLAGS=$(tsim_FLAGS_$f) &&) true
check_sim_flavour: check_sim_demo check_sim_op check_sim_run check_sim_sdl

check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: export context=hw_icarus
check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: check_hw_icarus_pre PERIODS.mk

check_hw_icarus_demo: export run=$(MAKE) $S -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile run_$*_demo | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD
check_sim_demo: export run=$(abspath $(BUILDDIR))/tsim$(EXE_SUFFIX) $(tsim_FLAGS) $(TOP)/ex/$*_demo.texe

check_sim_demo check_hw_icarus_demo: $(DEMOS:%=test_demo_%)
check_sim_op   check_hw_icarus_op:   $(OPS:%=  test_op_%  )
check_sim_run  check_hw_icarus_run:  $(RUNS:%= test_run_% )

ifeq ($(SDL),0)
check_sim_sdl: ;
else
check_sim_sdl: export SDL_VIDEODRIVER=dummy
check_sim_sdl: $(TOP)/ex/bm_mults.texe
	@$(MAKESTEP) -n "Running SDL test $(<F) ($(subst _sdl,,$(@:check_%=%))) ... "
	$(SILENCE)(cd $(TOP) ; $(abspath $(BUILDDIR))/tsim -n --recipe=prealloc -@ $(TOP)/plugins/sdl.rcp $<) && $(ECHO) ok
endif

# TODO make op tests take a fixed or predictable maximum amount of time
# The number of cycles necessary to run a code in Verilog simulation is
# determined by running it in tsim and multiplying the number of instructions
# executed by the number of cycles per instruction (currently 10). A margin of
# 2x is added to allow testcases not always to take exactly the same number of
# instructions to complete.
PERIODS.mk: $(OPS:%=$(TOP)/test/op/%.texe) $(DEMOS:%=$(TOP)/ex/%_demo.texe) $(RUNS:%=$(TOP)/test/run/%.texe) tsim$(EXE_SUFFIX)
	$(SILENCE)for f in $(filter-out tsim$(EXE_SUFFIX),$^) ; do echo PERIODS_`basename $${f/.texe/}`=`$(BUILDDIR)/tsim$(EXE_SUFFIX) -vv $$f | wc -l | while read b ; do dc -e "$$b 20*p" ; done` ; done > $@

check_sim_op: export stem=op
check_sim_run: export stem=run
check_sim_op check_sim_run: export run=$(abspath $(BUILDDIR)/tsim$(EXE_SUFFIX)) $(tsim_FLAGS) -vvvv $(TOP)/test/$(stem)/$*.texe | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'
check_hw_icarus_op: PERIODS.mk
check_hw_icarus_op check_hw_icarus_run: export run=$(MAKE) $S -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile run_$* VPATH=$(TOP)/test/op:$(TOP)/test/run BUILDDIR=$(abspath $(BUILDDIR)) PLUSARGS_EXTRA=+DUMPENDSTATE | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'

check_compile: tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Building tests from test/ ... "
	$(SILENCE)$(MAKE) $S -C $(TOP)/test && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Building examples from ex/ ... "
	$(SILENCE)$(MAKE) $S -C $(TOP)/ex && $(MAKESTEP) ok

dogfood: $(wildcard $(TOP)/test/pass_compile/*.tas $(TOP)/ex/*.tas*) tas$(EXE_SUFFIX)
	@$(ECHO) -n "Checking reversibility of assembly-disassembly ... "
	$(SILENCE)$(TOP)/scripts/dogfood.sh dogfood.$$$$.XXXXXX $(TAS) $(filter-out tas$(EXE_SUFFIX),$^) && $(ECHO) ok || $(ECHO) FAILED

