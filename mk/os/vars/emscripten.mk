# this file is included by the main Makefile automatically
export EXE_SUFFIX = .bc
EMCC = emcc
export CC := $(EMCC)
JIT = 0
USE_OWN_SEARCH = 1
EMCCFLAGS_LD += -s MODULARIZE=1
DYLIB_SUFFIX = .bc

BUILDDIR = $(TOP)/ui/web/build

TARGETS =# empty
TARGETS += $(BIN_TARGETS:$(EXE_SUFFIX)=.js) tcc.js
TARGETS += tcc.js
TARGETS += $(LIB_TARGETS:$(DYLIB_SUFFIX)=.js)

EXPORTED_FUNCTIONS += main

SDL_OPTS = \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s 'SDL2_IMAGE_FORMATS=["png"]' \
	#

ifeq ($(DEBUG),)
CC_OPT = -O2
CC_DEBUG =
CLOSURE_FLAGS = --closure 1
EMCCFLAGS_LD += -s ASSERTIONS=0 \
                -s LIBRARY_DEBUG=0 \
                -s DISABLE_EXCEPTION_CATCHING=1 \
                -s WARN_ON_UNDEFINED_SYMBOLS=0 \
                -s ERROR_ON_UNDEFINED_SYMBOLS=1 \
                #

else
CC_OPT = -O0
CC_DEBUG = -g
CLOSURE_FLAGS = --closure 0
EMCCFLAGS_LD += -s ASSERTIONS=2 \
                -s LIBRARY_DEBUG=0 \
                -s WARN_ON_UNDEFINED_SYMBOLS=1 \
                #
endif
EMCCFLAGS_LD += -s "EXPORT_NAME='Module_$*'"
EMCCFLAGS_LD += -s "EXPORTED_FUNCTIONS=[$(foreach f,$(EXPORTED_FUNCTIONS),'_$f',)]"

EMCCFLAGS_LD += $(CC_OPT) $(CC_DEBUG)
EMCCFLAGS_LD += $(CLOSURE_FLAGS)

PP_BUILD = $(TOP)/3rdparty/tinypp

TENYR_LIB_DIR ?= $(TOP)/lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --embed-file ,$(foreach m,$(TH_FILES),$m@$(notdir $m)))

RSRC_FILES = $(wildcard $(TOP)/rsrc/*.png)
RSRC_FLAGS = $(addprefix --embed-file ,$(foreach m,$(RSRC_FILES),$m@rsrc/$(notdir $m)))

clean_FILES += $(BUILDDIR)/*.bc $(BUILDDIR)/*.js.mem $(BUILDDIR)/*.js $(BUILDDIR)/*.data

