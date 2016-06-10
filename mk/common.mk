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
 CXXFLAGS += -g
 LDFLAGS  += -g
endif

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
CPPFLAGS += -D'PATH_COMPONENT_SEPARATOR_STR="$(PATH_COMPONENT_SEP)"'
CPPFLAGS += -D"PATH_SEPARATOR_CHAR=$(PATH_SEP_CHAR)"

ifeq ($(BITS),32)
 CFLAGS  += -m32
 LDFLAGS += -m32
endif

# Optimised build
ifeq ($(DEBUG),)
 CPPFLAGS += -DNDEBUG
 CFLAGS   += -O3
 CXXFLAGS += -O3
else
 CPPFLAGS += -DDEBUG=$(DEBUG)
 CFLAGS   += -fstack-protector -Wstack-protector
 CFLAGS   += -O0
 CXXFLAGS += -O0
endif

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -Wshadow $(PEDANTIC_FLAGS)
CXXFLAGS += -Wall -Wshadow
 #-Wextra $(PEDANTIC_FLAGS)
ifeq ($(PEDANTIC),)
 PEDANTIC_FLAGS ?= -pedantic
else
 PEDANTIC_FLAGS ?= -Werror -pedantic-errors
endif

COVERAGE_FLAGS = $(if $(GCOV),--coverage -O0)
CFLAGS   += $(COVERAGE_FLAGS)
CXXFLAGS += $(COVERAGE_FLAGS)
LDFLAGS  += $(COVERAGE_FLAGS)

CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

GIT = git --git-dir=$(TOP)/.git
CC  := $(CROSS_COMPILE)$(CC)
CXX := $(CROSS_COMPILE)$(CXX)
FLEX  ?= flex
BISON ?= bison -Werror

cc_flag_supp = $(shell $(CC) $1 -c -x c -o /dev/null /dev/null 2>/dev/null >/dev/null && echo $1)

TAS = $(runwrap)$(TOOLDIR)/tas$(EXE_SUFFIX)
TLD = $(runwrap)$(TOOLDIR)/tld$(EXE_SUFFIX)

DEVICES = ram sparseram serial spi
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVICES += spidummy spisd spi
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVICES:%=libtenyr%$(DYLIB_SUFFIX))

BIN_TARGETS += tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
LIB_TARGETS += $(PDEVLIBS)

TARGETS     ?= $(BIN_TARGETS) $(LIB_TARGETS)
RESOURCES   := $(wildcard $(TOP)/rsrc/64/*.png) \
               $(TOP)/rsrc/font.png \
               $(wildcard $(TOP)/plugins/*.rcp) \
               #

include $(TOP)/mk/os/default.mk
-include $(TOP)/mk/os/$(OS).mk
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
BUILD_NAME := $(shell $(GIT) describe --always --tags --match 'v?.?.?*' 2>/dev/null || $(ECHO) "unknown")

