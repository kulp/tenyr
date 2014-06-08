TOP ?= .

CC = $(CROSS_COMPILE)gcc
ECHO := $(shell which echo)
MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))

ifndef NDEBUG
 CFLAGS  += -g
 LDFLAGS += -g
endif

ifeq ($(WIN32),1)
 -include Makefile.Win32
else
 OS = $(shell uname -s)
 -include Makefile.$(OS)
 ifeq ($(_32BIT),1)
  CFLAGS  += -m32
  LDFLAGS += -m32
 endif
endif

INCLUDE_OS ?= $(TOP)/src/os/$(OS)

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra $(PEDANTIC_FLAGS)
ifeq ($(PEDANTIC),)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors
endif

gcc_flag_supported = $(shell gcc $1 -x c /dev/null 2>/dev/null >/dev/null && echo $1)

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'
# Use := to ensure the expensive underyling call is not repeated
NO_UNKNOWN_WARN_OPTS := $(call gcc_flag_supported,-Wno-unknown-warning-option)
CPPFLAGS += $(NO_UNKNOWN_WARN_OPTS)

# Optimised build
ifeq ($(DEBUG),)
 CPPFLAGS += -DNDEBUG
 CFLAGS   += -O3
else
 CPPFLAGS += -DDEBUG=$(DEBUG)
 CFLAGS   += -fstack-protector -Wstack-protector
endif

FLEX  = flex
BISON = bison -Werror

CFILES = $(wildcard src/*.c) $(wildcard src/devices/*.c)
GENDIR = $(TOP)/src/gen

VPATH += $(TOP)/src $(TOP)/src/devices $(GENDIR)
INCLUDES += $(TOP)/src $(GENDIR) $(INCLUDE_OS)

BUILD_NAME := $(shell git describe --tags --match 'v?.?.?*')
CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

ifneq ($(SDL),0)
SDL_VERSION = $(shell sdl2-config --version 2>/dev/null)
ifneq ($(SDL_VERSION),)
# Use := to ensure the expensive underyling call is not repeated
NO_C11_WARN_OPTS := $(call gcc_flag_supported,-Wno-c11-extensions)
libsdl%$(DYLIB_SUFFIX): CPPFLAGS += $(shell sdl2-config --cflags) $(NO_C11_WARN_OPTS)
libsdl%$(DYLIB_SUFFIX): LDLIBS   += $(shell sdl2-config --libs) -lSDL2_image
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
endif
endif

DEVICES = ram sparseram debugwrap serial spi eib
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVICES += spidummy spisd spi
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVOBJS:%,dy.o=lib%$(DYLIB_SUFFIX))

BIN_TARGETS ?= tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
LIB_TARGETS ?= $(PDEVLIBS)
TARGETS     ?= $(BIN_TARGETS) $(LIB_TARGETS)

.PHONY: all win32 win64 check
all: $(TARGETS)

win32: export _32BIT=1
win32 win64: export WIN32=1
# reinvoke make to ensure vars are set early enough
win32 win64:
	$(MAKE) $^

check: tsim tas tld
	@$(MAKESTEP) -n "Compiling tests from test/ ... "
	@$(MAKE) -s -B -C test && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Compiling examples from ex/ ... "
	@$(MAKE) -s -B -C ex && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Running qsort demo ... "
	@[ "$$(./tsim ex/qsort_demo.texe | sed -n 5p)" = "eight" ] && $(MAKESTEP) ok
	@$(MAKESTEP) -n "Running bsearch demo ... "
	@[ "$$(./tsim ex/bsearch_demo.texe | grep -v "not found" | wc -l | tr -d ' ')" = "11" ] && $(MAKESTEP) ok

TAS_OBJECTS  = common.o asmif.o asm.o obj.o $(GENDIR)/parser.o $(GENDIR)/lexer.o
TSIM_OBJECTS = common.o simif.o asm.o obj.o dbg.o ffi.o plugin.o \
               $(GENDIR)/debugger_parser.o $(GENDIR)/debugger_lexer.o $(DEVOBJS) sim.o param.o
TLD_OBJECTS  = common.o obj.o

tas$(EXE_SUFFIX):  $(TAS_OBJECTS)
tsim$(EXE_SUFFIX): $(TSIM_OBJECTS)
tld$(EXE_SUFFIX):  $(TLD_OBJECTS)

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
$(GENDIR)/debugger_parser.o $(GENDIR)/debugger_lexer.o \
$(GENDIR)/parser.o $(GENDIR)/lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

$(GENDIR)/lexer.o asmif.o dbg.o tas.o: $(GENDIR)/parser.h
tsim.o: $(GENDIR)/debugger_parser.h

$(GENDIR)/debugger_lexer.o: $(GENDIR)/debugger_parser.h
$(GENDIR)/debugger_lexer.h $(GENDIR)/debugger_lexer.c: debugger_lexer.l | $(GENDIR)
$(GENDIR)/debugger_parser.h $(GENDIR)/debugger_parser.c: debugger_parser.y $(GENDIR)/debugger_lexer.h | $(GENDIR)
$(GENDIR)/lexer.h $(GENDIR)/lexer.c: lexer.l | $(GENDIR)
$(GENDIR)/parser.h $(GENDIR)/parser.c: parser.y $(GENDIR)/lexer.h | $(GENDIR)

$(GENDIR):
	@mkdir -p $@

.PHONY: install upload
INSTALL_STEM ?= .
INSTALL_DIR  ?= $(INSTALL_STEM)/bin/$(BUILD_NAME)/$(shell $(CC) -dumpmachine)
install: tsim$(EXE_SUFFIX) tas$(EXE_SUFFIX) tld$(EXE_SUFFIX) $(PDEVLIBS)
	install -d $(INSTALL_DIR)
	install $^ $(INSTALL_DIR)

upload: tsim$(EXE_SUFFIX) tas$(EXE_SUFFIX) tld$(EXE_SUFFIX) | scripts/upload.pl
	$(realpath $|) "$(shell $(CC) -dumpmachine)" "$(shell git describe --tags)" $^

ifndef INHIBIT_DEPS
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CFILES:.c=.d)
endif

# TODO fix .d files ; something is causing many unnecessary rebuilds
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null && \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ && \
	rm -f $@.$$$$ || rm -f $@.$$$$
endif

CLEANFILES += $(TARGETS)
CLEANFILES += *.o *.d src/*.d src/devices/*.d $(GENDIR)/*.d $(GENDIR)/*.o $(PDEVOBJS)
clean::
	$(RM) $(CLEANFILES)

clobber:: clean
	$(RM) $(GENDIR)/debugger_parser.[ch] $(GENDIR)/debugger_lexer.[ch] $(GENDIR)/parser.[ch] $(GENDIR)/lexer.[ch]
	-rmdir $(GENDIR)
	$(RM) -r *.dSYM

clean clobber::
	-$(MAKE) -C ex $@
	-$(MAKE) -C test $@

##############################################################################

OUTPUT_OPTION ?= -o $@

COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
ifneq ($(MAKE_VERBOSE),)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
else
	@$(MAKESTEP) "[ CC ] $<"
	@$(COMPILE.c) $(OUTPUT_OPTION) $<
endif

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)

ifeq ($(SUPPRESS_BINARY_RULE),)
%$(EXE_SUFFIX): %.o
ifneq ($(MAKE_VERBOSE),)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
else
	@$(MAKESTEP) "[ LD ] $@"
	@$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endif
endif

$(GENDIR)/%.h $(GENDIR)/%.c: %.l
ifneq ($(MAKE_VERBOSE),)
	$(FLEX) --header-file=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
else
	@$(MAKESTEP) "[ FLEX ] $<"
	@$(FLEX) --header-file=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
endif

$(GENDIR)/%.h $(GENDIR)/%.c: %.y
ifneq ($(MAKE_VERBOSE),)
	$(BISON) --defines=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
else
	@$(MAKESTEP) "[ BISON ] $<"
	@$(BISON) --defines=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
endif

plugin,dy.o pluginimpl,dy.o $(PDEVOBJS): %,dy.o: %.c
ifneq ($(MAKE_VERBOSE),)
	$(COMPILE.c) -o $@ $<
else
	@$(MAKESTEP) "[ DYCC ] $<"
	@$(COMPILE.c) -o $@ $<
endif

$(PDEVLIBS): lib%$(DYLIB_SUFFIX): %,dy.o
ifneq ($(MAKE_VERBOSE),)
	$(LINK.c) -shared -o $@ $^ $(LDLIBS)
else
	@$(MAKESTEP) "[ DYLD ] $@"
	@$(LINK.c) -shared -o $@ $^ $(LDLIBS)
endif

