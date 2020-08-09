# this file is included by the main Makefile automatically
export EXE_SUFFIX = .js
EMCC = emcc
export CC := $(EMCC)
JIT = 0
USE_OWN_SEARCH = 1
LDFLAGS += -s MODULARIZE=1
LDFLAGS += -s NODERAWFS=1
DYLIB_SUFFIX = .js
tpp = cpp
CHECK_SW_TASKS = check_args check_behaviour check_compile check_sim check_obj check_forth # skip dogfood as it is quite expensive with emscripten

BUILDDIR = $(TOP)/ui/web/build

BIN_TARGETS += tcc$(EXE_SUFFIX)

EXPORTED_FUNCTIONS += main

DEFINES += 'MOUNT_POINT="/nodefs"'

SDL_OPTS = \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s 'SDL2_IMAGE_FORMATS=["png"]' \
	#

# Avoid choking on stdin / stdout macros.
CFLAGS += -Wno-disabled-macro-expansion

ifeq ($(DEBUG),)
CC_OPT = -O2
CC_DEBUG =
CLOSURE_FLAGS = --closure 1
LDFLAGS += -s ASSERTIONS=0 \
           -s LIBRARY_DEBUG=0 \
           -s DISABLE_EXCEPTION_CATCHING=1 \
           -s WARN_ON_UNDEFINED_SYMBOLS=0 \
           -s ERROR_ON_UNDEFINED_SYMBOLS=1 \
           #

else
CC_OPT = -O0
CC_DEBUG = -g
CLOSURE_FLAGS = --closure 0
LDFLAGS += -s ASSERTIONS=2 \
           -s LIBRARY_DEBUG=0 \
           -s WARN_ON_UNDEFINED_SYMBOLS=1 \
           #
endif
LDFLAGS += -s "EXPORT_NAME='Module_$*'"
null :=#
space :=$(null) $(null)#
comma :=$(null),#
LDFLAGS += -s "EXPORTED_FUNCTIONS=[$(subst $(space),$(comma),$(foreach f,$(EXPORTED_FUNCTIONS),'_$f'))]"

LDFLAGS += $(CC_OPT) $(CC_DEBUG)
LDFLAGS += $(CLOSURE_FLAGS)

PP_BUILD = $(TOP)/3rdparty/tinypp

TENYR_LIB_DIR ?= $(TOP)/lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --embed-file ,$(foreach m,$(TH_FILES),$m@$(notdir $m)))

RSRC_FILES = $(wildcard $(TOP)/rsrc/*.png)
RSRC_FLAGS = $(addprefix --embed-file ,$(foreach m,$(RSRC_FILES),$m@rsrc/$(notdir $m)))

clean_FILES += $(BUILDDIR)/*.js.mem $(BUILDDIR)/*.js $(BUILDDIR)/*.data

# runwrap might be nodejs on Debian
runwrap := node # trailing space required
