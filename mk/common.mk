# delete all build products built by a rule that exits nonzero
.DELETE_ON_ERROR:

ECHO := $(shell which echo)
EMPTY :=#

ifeq ($V,1)
 MAKESTEP = true
else
 MAKESTEP := $(ECHO)
.SILENT:
endif

export MAKESTEP

ifndef NDEBUG
 CFLAGS   += -g
 LDFLAGS  += -g
endif

# Set up -fsanitize= options
SANITIZE_FLAGS += $(SANITIZE:%=-fsanitize=%)
CFLAGS   += $(SANITIZE_FLAGS)
LDFLAGS  += $(SANITIZE_FLAGS)

ifeq ($(MEMCHECK),1)
 VALGRIND := $(shell which valgrind)
 ifeq ($(VALGRIND),)
  $(error MEMCHECK=1 was specified, but valgrind was not found)
 endif
 runwrap = $(VALGRIND) --leak-check=full --track-origins=yes --log-file=memcheck.$$$$ $(EMPTY)
endif

ifeq ($(PLATFORM),mingw)
 OS := Win32
else ifeq ($(PLATFORM),emscripten)
 OS := emscripten
else
 OS := $(shell uname -s)
endif
OS_PATHS = $(TOP)/src/os/$(OS) $(TOP)/src/os/default
INCLUDE_OS = $(OS_PATHS)
vpath %.c $(OS_PATHS)

LDFLAGS += $(LDFLAGS_$(OS))
LDLIBS  += $(LDLIBS_$(OS))

CPPFLAGS += -D"PATH_COMPONENT_SEPARATOR_CHAR='$(PATH_COMPONENT_SEP)'"
CPPFLAGS += -D'PATH_COMPONENT_SEPARATOR_STR="'$(PATH_COMPONENT_SEP)'"'
CPPFLAGS += -D"PATH_SEPARATOR_CHAR=$(PATH_SEP_CHAR)"

ifeq ($(BITS),32)
 CFLAGS  += -m32
 LDFLAGS += -m32
endif

# Optimised build
ifeq ($(DEBUG),)
 CPPFLAGS += -DNDEBUG
 CFLAGS   += -O3
else
 CPPFLAGS += -DDEBUG=$(DEBUG)
 CFLAGS   += -fstack-protector -Wstack-protector
 CFLAGS   += -O0
endif

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -Wshadow $(PEDANTIC_FLAGS)

include $(TOP)/mk/pedantic.mk

COVERAGE_FLAGS = $(if $(GCOV),--coverage -O0)
CFLAGS   += $(COVERAGE_FLAGS)
LDFLAGS  += $(COVERAGE_FLAGS)

CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

GIT = git --git-dir=$(TOP)/.git
FLEX  ?= flex
BISON ?= bison -Werror

cc_flag_supp = $(shell $(CC) $1 -c -x c -o /dev/null /dev/null 2>/dev/null >/dev/null && echo $1)

# In the presence of a non-null runwrap, $(build_tas) and $(tas) differ, so be
# careful to use the correct variable
build_tas  = $(TOOLDIR)/tas$(EXE_SUFFIX)
build_tld  = $(TOOLDIR)/tld$(EXE_SUFFIX)
build_tsim = $(TOOLDIR)/tsim$(EXE_SUFFIX)

tas  = $(runwrap)$(TOOLDIR)/tas$(EXE_SUFFIX)
tld  = $(runwrap)$(TOOLDIR)/tld$(EXE_SUFFIX)
tsim = $(runwrap)$(TOOLDIR)/tsim$(EXE_SUFFIX)
tpp  = $(CPP)

DEVICES = ram sparseram serial
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVICES:%=libtenyr%$(DYLIB_SUFFIX))

BIN_TARGETS += tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
LIB_TARGETS += $(PDEVLIBS)

TARGETS     ?= $(BIN_TARGETS) $(LIB_TARGETS)
RESOURCES   := $(wildcard $(TOP)/rsrc/64/*.png) \
               $(TOP)/rsrc/font10x15/invert.font10x15.png \
               $(wildcard $(TOP)/plugins/*.rcp) \
               #

include $(TOP)/mk/os/vars/default.mk
-include $(TOP)/mk/os/vars/$(OS).mk

# OS Makefiles might have set CROSS_COMPILE. Since we are using `:=` instead of
# `=`, order of assignment of variables matters.
CC  := $(CROSS_COMPILE)$(CC)

# Provide a short identifier (e.g. "clang" or "gcc") to switch some build-time
# behaviors (e.g. diagnostic fatalization).
COMPILER = $(shell echo __clang_version__ | $(CC) -E -P - | grep -qL __clang_version__ && echo gcc || echo clang)
include $(TOP)/mk/compiler/default.mk
-include $(TOP)/mk/compiler/$(COMPILER).mk

include $(TOP)/mk/sdl.mk
include $(TOP)/mk/jit.mk

# These definitions must come after OS includes
MACHINE := $(shell $(CC) -dumpmachine)
BUILDDIR ?= $(TOP)/build/$(MACHINE)
TOOLDIR := $(BUILDDIR)
ifeq ($(findstring command,$(origin $(BUILDDIR))),)
 ifeq ($(BUILDDIR),)
  override BUILDDIR := $(TOP)/build/$(MACHINE)
 endif
endif
# BUILD_DIR_NAME is used to guess version number when downloaded as a source
# tarball from GitHub
BUILD_DIR_NAME = $(notdir $(CURDIR))
# BUILD_DIR_VERS results in either a `vX.Y.Z` tag based on BUILD_DIR_NAME, or
# an empty string
BUILD_DIR_VERS = $(if $(findstring .,$(BUILD_DIR_NAME)),$(patsubst tenyr-%,v%,$(BUILD_DIR_NAME)))
# BUILD_DIR_DATE is a last-ditch effort to identify the build, by using the
# modification time of the build directory in unpunctuated ISO-8601 format.
# This is useful for builds that are not Git repositories and do not correspond
# to a GitHub release -- for example, a branch .ZIP download from GitHub.
BUILD_DIR_DATE = $(shell date -r "$(CURDIR)" +%Y%M%dT%H%M%S)
# BUILD_NAME prefers more precise measures, and falls back
BUILD_NAME := $(firstword \
        $(shell $(GIT) describe --always --tags --match 'v?.?.?*' 2>/dev/null) \
        $(BUILD_DIR_VERS) \
        $(BUILD_DIR_DATE) \
        unknown \
    )

