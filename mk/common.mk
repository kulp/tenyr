ECHO := $(shell which echo)

ifeq ($V,1)
 SILENCE =
 MAKESTEP = true
else
 S = -s
 SILENCE = @
 MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))
endif

ifndef NDEBUG
 CFLAGS  += -g
 LDFLAGS += -g
endif

ifeq ($(WIN32),1)
 OS := Win32
else
 OS := $(shell uname -s)
endif
INCLUDE_OS = $(TOP)/src/os/$(OS) $(TOP)/src/os/default

include $(TOP)/mk/os/default.mk
-include $(TOP)/mk/os/$(OS).mk

LDFLAGS += $(LDFLAGS_$(OS))
LDLIBS  += $(LDLIBS_$(OS))

CPPFLAGS += -D"PATH_SEPARATOR_CHAR=$(PATH_SEP_CHAR)"

ifeq ($(_32BIT),1)
 CFLAGS  += -m32
 LDFLAGS += -m32
endif

# Optimised build
ifeq ($(DEBUG),)
 CPPFLAGS += -DNDEBUG
 CFLAGS   += -O3
else
 CPPFLAGS += -O0 -DDEBUG=$(DEBUG)
 CFLAGS   += -fstack-protector -Wstack-protector
endif

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra $(PEDANTIC_FLAGS)
CXXFLAGS += -Wall
 #-Wextra $(PEDANTIC_FLAGS)
ifeq ($(PEDANTIC),)
 PEDANTIC_FLAGS ?= -pedantic
else
 PEDANTIC_FLAGS ?= -Werror -pedantic-errors
endif

ifneq ($(GCOV),)
 CFLAGS  += --coverage -O0
 CXXFLAGS  += --coverage -O0
 LDFLAGS += --coverage -O0
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

GIT = git --git-dir=$(TOP)/.git
CC := $(CROSS_COMPILE)$(CC)
FLEX  = flex
BISON = bison -Werror

cc_flag_supp = $(shell $(CC) $1 -x c /dev/null 2>/dev/null >/dev/null && echo $1)

MACHINE := $(shell $(CC) -dumpmachine)
BUILDDIR = $(TOP)/build/$(MACHINE)
TOOLDIR := $(BUILDDIR)
ifeq ($(findstring command,$(origin $(BUILDDIR))),)
 ifeq ($(BUILDDIR),)
  override BUILDDIR := $(TOP)/build/$(MACHINE)
 endif
endif
BUILD_NAME := $(shell $(GIT) describe --tags --match 'v?.?.?*' 2>/dev/null || $(ECHO) "unknown")

TAS = $(TOOLDIR)/tas
TLD = $(TOOLDIR)/tld

DEVICES = ram sparseram debugwrap serial spi
DEVOBJS = $(DEVICES:%=%.o)
# plugin devices
PDEVICES += spidummy spisd spi
PDEVOBJS = $(PDEVICES:%=%,dy.o)
PDEVLIBS = $(PDEVOBJS:%,dy.o=libtenyr%$(DYLIB_SUFFIX))

BIN_TARGETS ?= tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
LIB_TARGETS ?= $(PDEVLIBS)
TARGETS     ?= $(BIN_TARGETS) $(LIB_TARGETS)
RESOURCES   := $(wildcard $(TOP)/rsrc/64/*.png) \
               $(TOP)/rsrc/font.png \
               $(wildcard $(TOP)/plugins/*.rcp) \
               #

