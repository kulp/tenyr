makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := ..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

SHELL := $(shell which bash)

# Using `grep -q` stops processing as soon as a match is detected (potentially
# faster than processing all input), but causes upstream programs to exit (via
# EPIPE) early, resulting in unpredictable code coverage results if gcov is not
# able to flush before exit. Thus, we use a slower, full grep when coverage is
# enabled.
GREP = $(if $(GCOV),grep > /dev/null,grep -q)

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
	exec -a $* $($*) --help | \
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

coverage: coverage_html

LCOV ?= lcov
LCOVFLAGS = --config-file=$(TOP)/scripts/lcovrc

COVERAGE_RULE = check
coverage.info: LCOVFLAGS += --exclude="*/lexer.c"
coverage.info: LCOVFLAGS += --exclude="*/parser.c"
coverage.info: $(COVERAGE_RULE)
	$(LCOV) $(LCOVFLAGS) --capture --test-name $< --directory $(BUILDDIR) --output-file $@

coverage_html: coverage.info
	genhtml --output-directory $@ $^

check: check_sw check_hw
CHECK_SW_TASKS ?= check_sim check_obj
check_sw: check_ctest
check_sw: $(CHECK_SW_TASKS)

check_ctest: vpi
	cmake -S $(TOP) -B $(BUILDDIR)/ctest
	$(MAKE) --directory=$(TOP)/hw/icarus BUILDDIR=$(realpath $(BUILDDIR))
	cmake --build $(BUILDDIR)/ctest
	export PATH=$(abspath $(BUILDDIR)):$$PATH && cd $(BUILDDIR)/ctest && ctest

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

OPS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/op/*.tas)))
RUNS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/run/*.tas)))

# This test needs an additional object, as it tests the linker
$(TOP)/test/run/reloc_set.texe: $(TOP)/test/misc/reloc_set0.to
# This test needs imul compiled in
$(TOP)/test/run/test_imul.texe: $(TOP)/lib/imul.to
$(TOP)/test/run/reloc_shifts.texe: $(TOP)/test/misc/reloc_shifts0.to

check_hw: check_hw_icarus_run
vpi:
	$(MAKE) -C $(BUILDDIR) -f $(TOP)/hw/vpi/Makefile $@

test_run_% test_op_%: texe=$<

# Op tests are self-testing -- they must leave B with the value 0xffffffff if successful.
$(OPS:%=$(TOP)/test/op/%.texe): $(TOP)/test/op/%.texe: $(TOP)/test/op/%.to $(TOP)/test/misc/obj/args.to | $(build_tas)

test_op_%: $(TOP)/test/op/%.texe $(build_tas)
	@$(MAKESTEP) -n "Testing op `printf %-7s "'$*'"` ($(context)) ... "
	$(run) && $(MAKESTEP) ok

# Run tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .SECONDARY to indicate that run test files should *not* be deleted after
# one run, as they do not have random bits appended (yet).
.SECONDARY: $(RUNS:%=$(TOP)/test/run/%.texe)
test_run_%: %.texe $(build_tas) $(build_tld)
	@$(MAKESTEP) -n "Running test `printf %-20s "'$*'"` ($(context)) ... "
	$(run) && $(MAKESTEP) ok

tsim_FLAVOURS := interp interp_prealloc
tsim_FLAGS_interp =
tsim_FLAGS_interp_prealloc = --scratch --recipe=prealloc --recipe=serial --recipe=plugin
ifneq ($(JIT),0)
tsim_FLAVOURS += jit
tsim_FLAGS_jit = -rjit -ptsim.jit.run_count_threshold=2
check_sim::
	$(MAKE) -f $(TOP)/Makefile libtenyrjit$(DYLIB_SUFFIX)
endif

check_sim check_sim_run: export context=sim,$(flavour)
check_sim_run: $(build_tsim)
check_sim::
	$(foreach f,$(tsim_FLAVOURS),$(MAKE) -f $(makefile_path) check_sim_flavour flavour=$f tsim_FLAGS='$(tsim_FLAGS) $(tsim_FLAGS_$f)' &&) true
check_sim_flavour: check_sim_run

check_hw_icarus_run: export context=hw_icarus
check_hw_icarus_run: vpi

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
check_hw_icarus_run: $(SDL_RUNS:%=test_run_%)

ifneq ($(SDL),0)
RUNS += $(SDL_RUNS)
tsim_FLAGS += -p paths.share=$(call os_path,$(TOP)/)
tsim_FLAGS += -@ $(TOP)/plugins/sdl.rcp
endif

check_sim_run  check_hw_icarus_run:  $(RUNS:%= test_run_% )

vpath %.texe $(TOP)/test/op $(TOP)/ex $(TOP)/test/run

check_sim_run: export run=$(tsim) $(tsim_FLAGS) -p tsim.dump_end_state=1 $(texe) 2>&1 | grep -o 'B.[[:xdigit:]]\{8\}' | grep -q 'f\{8\}'
check_hw_icarus_run: export run=$(MAKE) -s --no-print-directory -C $(TOP)/hw/icarus run_$* VPATH=$(TOP)/test/op:$(TOP)/test/run BUILDDIR=$(abspath $(BUILDDIR)) PLUSARGS_EXTRA=+DUMPENDSTATE | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'

