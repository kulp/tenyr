makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

.DEFAULT_GOAL = all

DEVICES = ram sparseram debugwrap serial spi eib
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

INSTALL_STEM ?= $(TOP)
INSTALL_DIR  ?= $(INSTALL_STEM)/bin/$(BUILD_NAME)/$(MACHINE)

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'
# Use := to ensure the expensive underlying call is not repeated
NO_UNKNOWN_WARN_OPTS := $(call cc_flag_supp,-Wno-unknown-warning-option)
CPPFLAGS += $(NO_UNKNOWN_WARN_OPTS)

CFILES = $(wildcard src/*.c) $(wildcard src/devices/*.c)

VPATH += $(TOP)/src $(TOP)/src/devices
INCLUDES += $(TOP)/src $(INCLUDE_OS) $(BUILDDIR)

clean_FILES := $(addprefix $(BUILDDIR)/, \
                   *.o                   \
                   debugger_parser.[ch]  \
                   debugger_lexer.[ch]   \
                   parser.[ch]           \
                   lexer.[ch]            \
                   $(TARGETS)            \
               )#

tas_OBJECTS  = common.o asmif.o asm.o obj.o parser.o lexer.o
tsim_OBJECTS = common.o simif.o asm.o obj.o dbg.o ffi.o plugin.o \
               debugger_parser.o debugger_lexer.o $(DEVOBJS) sim.o param.o
tld_OBJECTS  = common.o obj.o

.PHONY: win32 win64
win32: export _32BIT=1
win32 win64: export WIN32=1
# reinvoke make to ensure vars are set early enough
win32 win64:
	$(MAKE) $^

showbuilddir:
	@echo $(abspath $(BUILDDIR))

clean clobber::
	-$(MAKE) -C $(TOP)/test $@

################################################################################
# Rerun make inside $(BUILDDIR) if we are not already building in the $(PWD)
DROP_TARGETS = win% showbuilddir clean clobber
ifneq ($(BUILDDIR),.)
all $(filter-out $(DROP_TARGETS),$(MAKECMDGOALS))::
	mkdir -p $(BUILDDIR)
	$(MAKE) TOOLDIR=$(BUILDDIR) BUILDDIR=. -C $(BUILDDIR) -f $(makefile_path) TOP=$(TOP) $@
else

.PHONY: all check coverage
all: $(TARGETS)

clobber_FILES += $(BUILDDIR)/*.gc??
coverage: CFLAGS  += --coverage
coverage: LDFLAGS += --coverage
coverage: coverage_html_src

clobber_FILES += $(BUILDDIR)/coverage.info
coverage.info: check
	lcov --capture --test-name $< --directory $(BUILDDIR) --output-file $@

coverage.info.trimmed: coverage.info
	lcov --output-file $@ --remove $< '*/debugger_*.*' --remove $< '*/dbg.c'

coverage.info.%: coverage.info.trimmed
	lcov --extract $< '*/$*/*' --output-file $@

CLOBBER_FILES += $(BUILDDIR)/coverage_html
coverage_html_%: coverage.info.%
	genhtml $< --output-directory $@

tools: tsim tas tld
check: dogfood tools
	@$(MAKESTEP) -n "Compiling tests from test/ ... "
	@$(MAKE) -s -B -C $(TOP)/test && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Compiling examples from ex/ ... "
	@$(MAKE) -s -B -C $(TOP)/ex && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Running qsort demo ... "
	@[ "$$($(BUILDDIR)/tsim $(TOP)/ex/qsort_demo.texe | sed -n 5p)" = "eight" ] && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Running bsearch demo ... "
	@[ "$$($(BUILDDIR)/tsim $(TOP)/ex/bsearch_demo.texe | grep -v "not found" | wc -l | tr -d ' ')" = "11" ] && $(MAKESTEP) ok

dogfood: $(wildcard $(TOP)/test/pass_compile/*.tas $(TOP)/ex/*.tas*) | tas
	@$(ECHO) -n "Checking reversibility of assembly-disassembly ... "
	@$(TOP)/scripts/dogfood.sh dogfood.$$$$.XXXXXX $(TAS) $^ | grep -qi failed && $(ECHO) FAILED || $(ECHO) ok

tas$(EXE_SUFFIX):  tas.o  $(tas_OBJECTS)
tsim$(EXE_SUFFIX): tsim.o $(tsim_OBJECTS)
tld$(EXE_SUFFIX):  tld.o  $(tld_OBJECTS)

asm.o: CFLAGS += -Wno-override-init

%,dy.o: CFLAGS += $(CFLAGS_PIC)

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

# don't complain about unused values that we might use in asserts
tas.o asm.o tsim.o sim.o simif.o dbg.o ffi.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
ffi.o asm.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-parameter
# link plugin-common data and functions into every plugin
$(PDEVLIBS): lib%$(DYLIB_SUFFIX): pluginimpl,dy.o plugin,dy.o

# flex-generated code we can't control warnings of as easily
debugger_parser.o debugger_lexer.o \
parser.o lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

lexer.o asmif.o dbg.o tas.o: parser.h
tsim.o dbg.o debugger_lexer.o: debugger_parser.h
debugger_parser.h debugger_parser.c: debugger_lexer.h
parser.h parser.c: lexer.h

.PHONY: install
install: tsim$(EXE_SUFFIX) tas$(EXE_SUFFIX) tld$(EXE_SUFFIX) $(PDEVLIBS)
	install -d $(INSTALL_DIR)
	install $^ $(INSTALL_DIR)

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

