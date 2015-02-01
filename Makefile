makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

lockfile = lockfile

# ensure directory printing doesn't mess up check rules
GNUMAKEFLAGS += --no-print-directory

.DEFAULT_GOAL = all

DEVICES = ram sparseram debugwrap serial spi
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVICES += spidummy spisd spi
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVOBJS:%,dy.o=lib%$(DYLIB_SUFFIX))

ifneq ($(SDL),0)
SDL_VERSION = $(shell sdl2-config --version 2>/dev/null)
ifneq ($(SDL_VERSION),)
# Use := to ensure the expensive underyling call is not repeated
NO_C11_WARN_OPTS := $(call cc_flag_supp,-Wno-c11-extensions)
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
libsdl%$(DYLIB_SUFFIX) $(PDEVICES_SDL:%=devices/%.d): CPPFLAGS += $(shell sdl2-config --cflags) $(NO_C11_WARN_OPTS)
libsdl%$(DYLIB_SUFFIX): LDLIBS += $(shell sdl2-config --libs) -lSDL2_image
$(PDEVICES_SDL:%=%,dy.o): PEDANTIC_FLAGS :=
endif
endif

BIN_TARGETS ?= tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
LIB_TARGETS ?= $(PDEVLIBS)
TARGETS     ?= $(BIN_TARGETS) $(LIB_TARGETS)
RESOURCES   := $(wildcard $(TOP)/rsrc/64/*.png) \
               $(TOP)/rsrc/font.png \
               $(wildcard $(TOP)/plugins/*.rcp) \
               #

INSTALL_DIR ?= /usr/local

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'
# Use := to ensure the expensive underlying call is not repeated
NO_UNKNOWN_WARN_OPTS := $(call cc_flag_supp,-Wno-unknown-warning-option)
CPPFLAGS += $(NO_UNKNOWN_WARN_OPTS)

SOURCEFILES = $(wildcard $(TOP)/src/*.c $(TOP)/src/devices/*.c)

VPATH += $(TOP)/src $(TOP)/src/devices
INCLUDES += $(TOP)/src $(INCLUDE_OS) $(BUILDDIR)

clean_FILES := $(addprefix $(BUILDDIR)/, \
                   *.o                   \
                   *.d                   \
                   parser.[ch]           \
                   lexer.[ch]            \
                   $(TARGETS)            \
               )#

tas_OBJECTS  = common.o asmif.o asm.o obj.o parser.o lexer.o
tsim_OBJECTS = common.o simif.o asm.o obj.o plugin.o \
               $(DEVOBJS) sim.o param.o
tld_OBJECTS  = common.o obj.o

.PHONY: win32 win64
win32: export _32BIT=1
win32 win64: export WIN32=1
# reinvoke make to ensure vars are set early enough
win32 win64:
	$(MAKE) $^

showbuilddir:
	@echo $(abspath $(BUILDDIR))

.PHONY: distclean
distclean:: clobber
	$(RM) -r install/ build/

clean clobber::
	-rmdir $(BUILDDIR) build # fail, ignore if non-empty
	-$(MAKE) -C $(TOP)/test $@
	-$(MAKE) -C $(TOP)/hw/icarus $@
	-$(MAKE) -C $(TOP)/hw/xilinx $@

clobber_FILES += $(BUILDDIR)/*.gc??
clobber_FILES += $(BUILDDIR)/coverage.info
clobber_FILES += $(BUILDDIR)/PERIODS.mk
clobber::
	-$(MAKE) -C $(TOP)/ex $@

################################################################################
# Rerun make inside $(BUILDDIR) if we are not already building in the $(PWD)
DROP_TARGETS = win% showbuilddir clean clobber distclean
ifneq ($(BUILDDIR),.)
all $(filter-out $(DROP_TARGETS),$(MAKECMDGOALS))::
	mkdir -p $(BUILDDIR)
	$(MAKE) TOOLDIR=$(BUILDDIR) BUILDDIR=. -C $(BUILDDIR) -f $(makefile_path) TOP=$(TOP) $@
else

.PHONY: all check coverage doc
all: $(TARGETS)
doc: tas_usage tsim_usage tld_usage

%_usage: %$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Generating usage description for $* ... "
	$(SILENCE)PATH=$(BUILDDIR) $< --help | sed 's/^/    /;/version/s/-[0-9][0-9]*-g[[:xdigit:]]\{7\}/.../' > $(TOP)/wiki/$*--help.md && $(MAKESTEP) ok

.PHONY: gzip zip
gzip zip: export CREATE_TEMP_INSTALL_DIR=1

ifeq ($(CREATE_TEMP_INSTALL_DIR),1)
gzip: tenyr-$(BUILD_NAME).tar.gz
zip: tenyr-$(BUILD_NAME).zip

tenyr-$(BUILD_NAME).zip tenyr-$(BUILD_NAME).tar.gz: INSTALL_PRE := $(shell mktemp -d tenyr.dist.XXXXXX)
tenyr-$(BUILD_NAME).zip tenyr-$(BUILD_NAME).tar.gz: INSTALL_DIR := $(INSTALL_PRE)/tenyr-$(BUILD_NAME)

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

coverage: CFLAGS  += --coverage
coverage: LDFLAGS += --coverage
coverage: coverage_html_src

coverage.info: check_sw
	lcov --capture --test-name $< --directory $(BUILDDIR) --output-file $@

coverage.info.trimmed: coverage.info
	lcov --output-file $@ --remove $< --remove $<

coverage.info.%: coverage.info.trimmed
	lcov --extract $< '*/$*/*' --output-file $@

CLOBBER_FILES += $(BUILDDIR)/coverage_html
coverage_html_%: coverage.info.%
	genhtml $< --output-directory $@

check: check_sw check_hw
check_sw: check_compile check_sim dogfood

define LOCK
	$(lockfile) -r2 $(@D)/lock.$(@F)
endef

define UNLOCK
	-$(RM) $(@D)/lock.$(@F)
endef

vpath %_demo.tas.cpp $(TOP)/ex
DEMOS = qsort bsearch trailz
DEMOFILES = $(DEMOS:%=$(TOP)/ex/%_demo.texe)
OPS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/op/*.tas)))
RUNS = $(subst .tas,,$(notdir $(wildcard $(TOP)/test/run/*.tas)))
#TESTS = $(patsubst $(TOP)/test/%.tas,%,$(wildcard $(TOP)/test/*/*.tas))
$(DEMOFILES): %_demo.texe: %_demo.tas.cpp | tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	$(SILENCE)$(call LOCK)
	@$(MAKESTEP) -n "Building $(*F) demo ... "
	$(SILENCE)$(MAKE) -s -C $(@D) $(@F) && $(MAKESTEP) ok
	$(SILENCE)$(call UNLOCK)

check_hw:
	@$(MAKESTEP) -n "Checking for Icarus Verilog ... "
	$(SILENCE)icarus="$$($(MAKE) --no-print-directory -s -i -C $(TOP)/hw/icarus has-icarus)" ; \
	if [ -x "$$icarus" ] ; then \
		$(MAKESTEP) $$icarus ; \
		$(MAKE) -C $(BUILDDIR) -f $(makefile_path) check_hw_icarus_op check_hw_icarus_demo ; \
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
test_demo_%: $(TOP)/ex/%_demo.texe PERIODS.mk
	@$(MAKESTEP) -n "Running $* demo ($(context)) ... "
	$(SILENCE)[ "$$($(call run,$*) | $(verify))" = "$(result)" ] && $(MAKESTEP) ok

# Op tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .INTERMEDIATE to indicate that op test files should be deleted after one
# run -- they have random bits appended which should be regenerated each time.
.INTERMEDIATE: $(OPS:%=$(TOP)/test/op/%.texe)
$(TOP)/test/op/%.texe: $(TOP)/test/op/%.tas | tas$(EXE_SUFFIX)
	$(SILENCE)(cat $< ; hexdump -n12 -e'".word 0x%08x; "' /dev/urandom) | $(BUILDDIR)/tas -o $@ -

test_op_%: $(TOP)/test/op/%.texe PERIODS.mk | tas$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Testing op `printf %-7s "'$*'"` ($(context)) ... "
	$(SILENCE)$(run) && $(MAKESTEP) ok

# Run tests are self-testing -- they must leave B with the value 0xffffffff.
# Use .SECONDARY to indicate that run test files should *not* be deleted after
# one run, as they do not have random bits appended (yet).
.SECONDARY: $(RUNS:%=$(TOP)/test/run/%.texe)
test_run_%: $(TOP)/test/run/%.texe PERIODS.mk | tas$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Running test `printf %-10s "'$*'"` ($(context)) ... "
	$(SILENCE)$(run) && $(MAKESTEP) ok

check_hw_icarus_pre:
	@$(MAKESTEP) -n "Building hardware simulator ... "
	$(SILENCE)$(MAKE) -s -C $(TOP)/hw/icarus tenyr && $(MAKESTEP) ok

check_sim check_sim_demo check_sim_op check_sim_run: export context=sim
check_sim_demo check_sim_op check_sim_run: tsim$(EXE_SUFFIX)
check_sim: check_sim_demo check_sim_op check_sim_run

check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: export context=hw_icarus
check_hw_icarus_demo check_hw_icarus_op check_hw_icarus_run: check_hw_icarus_pre PERIODS.mk

check_hw_icarus_demo: export run=$(MAKE) -s -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile run_$*_demo | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD
check_sim_demo: export run=$(abspath $(BUILDDIR))/tsim$(EXE_SUFFIX) $(TOP)/ex/$*_demo.texe

check_sim_demo check_hw_icarus_demo: $(DEMOS:%=test_demo_%)
check_sim_op   check_hw_icarus_op:   $(OPS:%=  test_op_%  )
check_sim_run  check_hw_icarus_run:  $(RUNS:%= test_run_% )

# TODO make op tests take a fixed or predictable maximum amount of time
# The number of cycles necessary to run a code in Verilog simulation is
# determined by running it in tsim and multiplying the number of instructions
# executed by the number of cycles per instruction (currently 10). A margin of
# 2x is added to allow testcases not always to take exactly the same number of
# instructions to complete.
PERIODS.mk: $(OPS:%=$(TOP)/test/op/%.texe) $(DEMOS:%=$(TOP)/ex/%_demo.texe) $(RUNS:%=$(TOP)/test/run/%.texe) | tsim$(EXE_SUFFIX)
	$(SILENCE)for f in $^ ; do echo PERIODS_`basename $${f/.texe/}`=`$(BUILDDIR)/tsim$(EXE_SUFFIX) -vv $$f | wc -l | while read b ; do dc -e "$$b 20*p" ; done` ; done > $@

check_sim_op: export stem=op
check_sim_run: export stem=run
check_sim_op check_sim_run: export run=$(abspath $(BUILDDIR)/tsim$(EXE_SUFFIX)) -vvvv $(TOP)/test/$(stem)/$*.texe | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'
check_hw_icarus_op: PERIODS.mk
check_hw_icarus_op check_hw_icarus_run: export run=$(MAKE) -s -C $(TOP)/hw/icarus -f $(abspath $(BUILDDIR))/PERIODS.mk -f Makefile run_$* VPATH=$(TOP)/test/op:$(TOP)/test/run PLUSARGS_EXTRA=+DUMPENDSTATE | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD | grep -o 'B.[[:xdigit:]]\{8\}' | tail -n1 | grep -q 'f\{8\}'

check_compile: | tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Building tests from test/ ... "
	$(SILENCE)$(MAKE) -s -C $(TOP)/test && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Building examples from ex/ ... "
	$(SILENCE)$(MAKE) -s -C $(TOP)/ex && $(MAKESTEP) ok

dogfood: $(wildcard $(TOP)/test/pass_compile/*.tas $(TOP)/ex/*.tas*) | tas$(EXE_SUFFIX)
	@$(ECHO) -n "Checking reversibility of assembly-disassembly ... "
	$(SILENCE)$(TOP)/scripts/dogfood.sh dogfood.$$$$.XXXXXX $(TAS) $^ | grep -qi failed && $(ECHO) FAILED || $(ECHO) ok

tas$(EXE_SUFFIX):  tas.o  $(tas_OBJECTS)
tsim$(EXE_SUFFIX): tsim.o $(tsim_OBJECTS)
tld$(EXE_SUFFIX):  tld.o  $(tld_OBJECTS)

asm.o: CFLAGS += -Wno-override-init

%,dy.o: CFLAGS += $(CFLAGS_PIC)

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

# don't complain about unused values that we might use in asserts
tas.o asm.o tsim.o sim.o simif.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
asm.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-parameter
# link plugin-common data and functions into every plugin
$(PDEVLIBS): lib%$(DYLIB_SUFFIX): pluginimpl,dy.o plugin,dy.o

# flex-generated code we can't control warnings of as easily
parser.o lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

lexer.o asmif.o tas.o: parser.h
parser.h parser.c: lexer.h

.PHONY: install local-install
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

ifeq ($(filter $(DROP_TARGETS),$(MAKECMDGOALS)),)
-include $(patsubst $(TOP)/src/%.c,$(BUILDDIR)/%.d,$(SOURCEFILES))
endif

##############################################################################

plugin,dy.o pluginimpl,dy.o $(PDEVOBJS): %,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(SILENCE)$(COMPILE.c) -o $@ $<

$(PDEVLIBS): lib%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(SILENCE)$(LINK.c) -shared -o $@ $^ $(LDLIBS)
endif # BUILDDIR

