# delete all build products built by a rule that exits nonzero
.DELETE_ON_ERROR:

ifneq ($V,1)
.SILENT:
endif

ARCH := $(shell uname -m)
OS := $(shell uname -s)
OS_PATHS = src/os/$(OS) src/os/default
INCLUDE_OS = $(OS_PATHS)
vpath %.c $(OS_PATHS)

LDFLAGS += $(LDFLAGS_$(OS))
LDLIBS  += $(LDLIBS_$(OS))

CPPFLAGS += -D"PATH_SEPARATOR_CHAR=$(PATH_SEP_CHAR)"

CPPFLAGS += -DNDEBUG
CFLAGS   += -O3

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -Wshadow

CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

LEX = flex
YACC = bison
YFLAGS = -Werror

DEVICES = ram sparseram serial zero_word
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVICES:%=libtenyr%$(DYLIB_SUFFIX))

BIN_TARGETS += tas tsim tld
LIB_TARGETS += $(PDEVLIBS)

TARGETS      = $(BIN_TARGETS) $(LIB_TARGETS) $(TEST_TARGETS)
RESOURCES   := $(wildcard rsrc/64/*.png) \
               rsrc/font10x15/invert.font10x15.png \
               $(wildcard plugins/*.rcp) \
               #

include mk/os/vars/default.mk
-include mk/os/vars/$(OS).mk

# BUILD_DIR_DATE is a last-ditch effort to identify the build, by using the
# modification time of the build directory in unpunctuated ISO-8601 format.
# This is useful for builds that are not Git repositories and do not correspond
# to a GitHub release -- for example, a branch .ZIP download from GitHub.
BUILD_DIR_DATE = $(shell date -r "$(CURDIR)" +%Y%M%dT%H%M%S)
# BUILD_NAME prefers more precise measures, and falls back
BUILD_NAME := $(firstword \
        $(shell git describe --always --tags --match 'v?.?.?*' 2>/dev/null) \
        $(BUILD_DIR_DATE) \
        unknown \
    )

# Set up dependency generation flags.
%.o: CPPFLAGS += -MMD -MT '$*.o $*,dy.o $*.d' -MF $*.d

%.o: %.c
	@echo "[ CC ] $(<F)"
	$(COMPILE.c) -o $@ $<

%: %.o
	@echo "[ LD ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

%,dy.o: CFLAGS += $(CFLAGS_PIC)
%,dy.o: %.c
	@echo "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

libtenyr%$(DYLIB_SUFFIX): %,dy.o
	@echo "[ DYLD ] $@"
	$(LINK.c) -shared -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@echo "[ FLEX ] $(<F)"
	# `sed` here hacks around an issue where gcov gets line numbers off by one
	# after the rules section
	$(LEX) --header-file=$*.h --outfile=$*.c $<

%.h %.c: %.y
	@echo "[ BISON ] $(<F)"
	$(YACC.y) --defines=$*.h -o $*.c $<

clean::
	$(RM) -rf $($@_FILES)

# ensure directory printing doesn't mess up check rules
GNUMAKEFLAGS += --no-print-directory

.DEFAULT_GOAL = all

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'

SOURCEFILES = $(wildcard src/*.c src/devices/*.c)
VPIFILES = $(wildcard hw/vpi/*.c)

VPATH += src src/devices hw/vpi
INCLUDES += src $(INCLUDE_OS) .

clean_FILES = \
                   *.o                   \
                   *.d                   \
                   parser.[ch]           \
                   lexer.[ch]            \
                   $(TARGETS)            \
                   $(SOURCEFILES:src/%.c=%.d) \
                   $(VPIFILES:hw/vpi/%.c=%.d) \
                   random random.*       \
                #

common_OBJECTS = common.o param.o $(patsubst %.c,%.o,$(notdir $(wildcard $(OS_PATHS:%=%/*.c)))) \
                 stream.o
shared_OBJECTS = common.o
tas_OBJECTS    = $(common_OBJECTS) asmif.o asm.o obj.o parser.o lexer.o
tsim_OBJECTS   = $(common_OBJECTS) simif.o asm.o obj.o plugin.o \
                 $(DEVOBJS) sim.o
tld_OBJECTS    = $(common_OBJECTS) obj.o

################################################################################
DROP_TARGETS = clean

all: $(TARGETS)

-include mk/os/rules/$(OS).mk

tas:  tas.o  $(tas_OBJECTS)
tsim: tsim.o $(tsim_OBJECTS)
tld:  tld.o  $(tld_OBJECTS)

# used to apply to .o only but some make versions built directly from .c
tas tsim tld: DEFINES += BUILD_NAME='$(BUILD_NAME)'

# don't complain about unused state
asm.o asmif.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-parameter
# link plugin-common data and functions into every plugin
$(PDEVLIBS): libtenyr%$(DYLIB_SUFFIX): pluginimpl,dy.o $(shared_OBJECTS:%.o=%,dy.o)

# We cannot control some aspects of the generated lexer or parser.
lexer.o parser.o: CFLAGS += -Wno-error
lexer.o: CFLAGS += -Wno-missing-prototypes
lexer.o parser.o: CFLAGS += -Wno-unused-macros
lexer.o: CFLAGS += -Wno-shorten-64-to-32
lexer.o: CFLAGS += -Wno-conversion
lexer.o: CPPFLAGS += -Wno-disabled-macro-expansion
lexer.o: CPPFLAGS += -Wno-documentation
lexer.o: CFLAGS += -Wno-missing-noreturn
lexer.o parser.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter
# flex-generated code needs POSIX source for fileno()
lexer.o: CPPFLAGS += -D_POSIX_SOURCE

lexer.o asmif.o tas.o: parser.h
parser.h parser.c: lexer.h

ifeq ($(filter $(DROP_TARGETS),$(MAKECMDGOALS)),)
-include $(patsubst src/%.c,%.d,$(SOURCEFILES))
-include $(patsubst hw/vpi/%.c,%.d,$(VPIFILES))
-include lexer.d
-include parser.d
endif

# We make some targets depend on `all` because it assuages some issues with a
# top-level `make -j check` (for example) where many copies of tas (for
# example) are built simultaneously, sometimes overwriting each other
# non-atomically
check: all

check: vpi jit icarus
	cmake -S . -B ctest -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS}
	cmake --build ctest
	export PATH=$(abspath .):$$PATH && cd ctest && ctest

ifneq ($(ICARUS),0)
%.vpi: INCLUDES += src
%.vpi: INCLUDES += $(INCLUDE_OS)

vpath %.c src
vpath %.c hw/vpi

all: vpi

%.vpi: CFLAGS  += $(shell iverilog-vpi --cflags 2> /dev/null | sed s/-no-cpp-precomp//)
%.vpi: CFLAGS  += -Wno-strict-prototypes
%.vpi: LDFLAGS += $(shell iverilog-vpi --ldflags 2> /dev/null)
%.vpi: LDLIBS  += $(shell iverilog-vpi --ldlibs 2> /dev/null)
%.vpi: %,dy.o
	@echo "[ VPI ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

.PHONY: vpi
vpi: vpidevices.vpi
vpidevices.vpi: callbacks,dy.o vpiserial,dy.o load,dy.o sim,dy.o asm,dy.o obj,dy.o common,dy.o param,dy.o stream,dy.o

icarus: tenyr
tenyr: CFLAGS += -Wall -Wextra -Wshadow -pedantic-errors -std=c99 $(VPI_CFLAGS)
tenyr: CFLAGS += -Isrc

tenyr: INCLUDES += $(INCLUDE_OS)

clean_FILES += tenyr *.o

all: tenyr

vpath %.v hw/verilog hw/verilog/sim hw/icarus
vpath %.v 3rdparty/wb_intercon/rtl/verilog

tenyr: VFLAGS += -Ihw/verilog -g2005-sv
tenyr: VFLAGS += -DSIM

# We use IVERILOG_VPI_MODULE_PATH rather than -L in order to maintain backward
# compatibility with Icarus v10.
tenyr: export IVERILOG_VPI_MODULE_PATH=$(realpath .)
tenyr: VFLAGS += -m vpidevices
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       gpio.v                                       \
       wb_mux.v                                     \
       #

##############################################################################

OUTPUT_OPTION ?= -o $@

%: %.v
	@echo "[ IVERILOG ] $@"
	iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

else
vpi: ; # VPI support not enabled
icarus: ; # Icarus not enabled
endif
