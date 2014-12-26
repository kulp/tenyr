makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

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
libsdl%$(DYLIB_SUFFIX): CPPFLAGS += $(shell sdl2-config --cflags) $(NO_C11_WARN_OPTS)
libsdl%$(DYLIB_SUFFIX): LDLIBS   += $(shell sdl2-config --libs) -lSDL2_image
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
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

CFILES = $(wildcard src/*.c) $(wildcard src/devices/*.c)

VPATH += $(TOP)/src $(TOP)/src/devices
INCLUDES += $(TOP)/src $(INCLUDE_OS) $(BUILDDIR)

clean_FILES := $(addprefix $(BUILDDIR)/, \
                   *.o                   \
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
	$(RM) -r install/

clean clobber::
	-rmdir $(BUILDDIR) build # fail, ignore if non-empty
	-$(MAKE) -C $(TOP)/test $@

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

.PHONY: all check coverage
all: $(TARGETS)

.PHONY: gzip zip
gzip: tenyr-$(BUILD_NAME).tar.gz
zip: tenyr-$(BUILD_NAME).zip

tenyr-$(BUILD_NAME).tar.gz: INSTALL_DIR := $(shell mktemp -d tenyr.gzip.XXXXXX)/tenyr-$(BUILD_NAME)
tenyr-$(BUILD_NAME).tar.gz: install
	tar zcf $@ -C $(INSTALL_DIR)/.. .

tenyr-$(BUILD_NAME).zip: INSTALL_DIR := $(shell mktemp -d tenyr.zip.XXXXXX)/tenyr-$(BUILD_NAME)
tenyr-$(BUILD_NAME).zip: install
	orig=$(abspath $@) && (cd $(INSTALL_DIR)/.. ; zip -r $$orig .)

clobber_FILES += $(BUILDDIR)/*.gc??
coverage: CFLAGS  += --coverage
coverage: LDFLAGS += --coverage
coverage: coverage_html_src

clobber_FILES += $(BUILDDIR)/coverage.info
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

vpath %_demo.tas.cpp $(TOP)/ex
DEMOS = qsort bsearch
DEMOFILES = $(DEMOS:%=$(TOP)/ex/%_demo.texe)
$(DEMOFILES): %_demo.texe: %_demo.tas.cpp | tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Building $(*F) demo ... "
	$(SILENCE)$(MAKE) -s -C $(@D) $(@F) && $(MAKESTEP) ok

check_hw:
	@$(MAKESTEP) -n "Checking for Icarus Verilog ... "
	$(SILENCE)icarus="$$($(MAKE) --no-print-directory -s -i -C $(TOP)/hw/icarus has-icarus)" ; \
	if [ -x "$$icarus" ] ; then \
		$(MAKESTEP) $$icarus ; \
		$(MAKE) -C $(BUILDDIR) -f $(makefile_path) check_hw_icarus ; \
	else \
		$(MAKESTEP) "not found" ; \
	fi

PERIODS_qsort   = 70000
PERIODS_bsearch = 400000

run_demo_qsort:   verify = sed -n 5p
run_demo_qsort:   result = eight
run_demo_bsearch: verify = grep -v "not found" | wc -l | tr -d ' '
run_demo_bsearch: result = 11
run_demo_%:
	@$(MAKESTEP) -n "Running $* demo ($(context)) ... "
	$(SILENCE)[ "$$($(call run,$*) | $(verify))" = "$(result)" ] && $(MAKESTEP) ok

check_hw_icarus_pre:
	@$(MAKESTEP) -n "Building hardware simulator ... "
	$(SILENCE)$(MAKE) -s -C $(TOP)/hw/icarus tenyr && $(MAKESTEP) ok

check_sim: export run=$(abspath $(BUILDDIR))/tsim$(EXE_SUFFIX) $(TOP)/ex/$$*_demo.texe
check_sim: tsim$(EXE_SUFFIX)

check_hw_icarus: export run=$(MAKE) -s -C $(TOP)/hw/icarus run_$$*_demo PERIODS=$$(PERIODS_$$*) | grep -v -e ^WARNING: -e ^ERROR: -e ^VCD
check_hw_icarus: check_hw_icarus_pre

check_sim check_hw_icarus: $(DEMOFILES)
	$(SILENCE)$(MAKE) -C $(TOP) context=$@ $(foreach d,$(DEMOS),run_demo_$d)

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
-include $(CFILES:.c=.d)
endif

##############################################################################

plugin,dy.o pluginimpl,dy.o $(PDEVOBJS): %,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(SILENCE)$(COMPILE.c) -o $@ $<

$(PDEVLIBS): lib%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(SILENCE)$(LINK.c) -shared -o $@ $^ $(LDLIBS)
endif # BUILDDIR

