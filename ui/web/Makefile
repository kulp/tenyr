makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
HERE := $(dir $(makefile_path))
TOP ?= $(HERE)/main_build
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

BIN_TARGETS = tcc.js tas.js tsim.js tld.js
TARGETS = $(BIN_TARGETS)
EXE_SUFFIX = .bc
CPPFLAGS += -D"PATH_COMPONENT_SEPARATOR_CHAR='/'"
CPPFLAGS += -D'BUILD_NAME=$(BUILD_NAME)'
CPPFLAGS += -D'DYLIB_SUFFIX=".so"'

.PHONY: all
all:: $(BIN_TARGETS)

MAIN_BUILD_DIR := $(shell $(MAKE) -s -C main_build showbuilddir)

CHECK_PATHS = src main_build/src $(MAIN_BUILD_DIR) main_build/src/os/Linux
vpath %.h $(CHECK_PATHS)
vpath %.c $(CHECK_PATHS) main_build/src/devices
vpath %.bc pp_build
INCLUDES += $(CHECK_PATHS) main_build/src/os/default

EMCC = emcc

ifeq ($(EMRUN),1)
EMCCFLAGS_LD += --emrun
endif

################################################################################
# General rules

%.bc: export CC = $(EMCC)
%.bc: export EXE_SUFFIX = .bc
%.bc: CPPFLAGS += -DEMSCRIPTEN
%.bc: CPPFLAGS += -Wno-warn-absolute-paths

EMCCFLAGS_LD += -s MODULARIZE=1

ifeq ($(DEBUG),)
CC_OPT = -O2
CC_DEBUG =
CLOSURE_FLAGS = --closure 1
%.js: EMCCFLAGS_LD += -s ASSERTIONS=0 \
                      -s LIBRARY_DEBUG=0 \
                      -s DISABLE_EXCEPTION_CATCHING=1 \
                      -s WARN_ON_UNDEFINED_SYMBOLS=0 \
                      #
else
CC_OPT = -O0
CC_DEBUG = -g
CLOSURE_FLAGS = --closure 0
%.js: EMCCFLAGS_LD += -s ASSERTIONS=2 \
                      -s LIBRARY_DEBUG=0 \
                      #
endif
%.js: EMCCFLAGS_LD += $(CC_OPT) $(CC_DEBUG)
%.js: EMCCFLAGS_LD += $(CLOSURE_FLAGS)

%.js: EMCCFLAGS_LD += -s "EXPORT_NAME='Module_$*'"
%.js: EMCCFLAGS_LD += -s INVOKE_RUN=0 -s NO_EXIT_RUNTIME=1

%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.js: %.bc
	@$(MAKESTEP) "[ EM-LD ] $@"
	$(EMCC) $(EMCCFLAGS_LD) $< $(LDLIBS) -o $@

################################################################################
# Specific rules

parser.h parser.c lexer.c lexer.h:
	$(MAKE) CC=cc -C $(TOP) $@
	cp -p $(MAIN_BUILD_DIR)/$(@F) $@

asmif.o: CFLAGS += -Wno-unused-parameter
asmif.o tas.o: parser.h lexer.h

tcc.js: pp_build/tcc.bc

TENYR_LIB_DIR ?= lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --preload-file ,$(foreach m,$(TH_FILES),$m@$(notdir $m)))
tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

pp_build/%:
	$(MAKE) -C pp_build $*

TH_FILES_RSRC = $(wildcard main_build/rsrc/*.png)
TH_FLAGS_RSRC = $(addprefix --preload-file ,$(foreach m,$(TH_FILES_RSRC),$m@rsrc/$(notdir $m)))
tsim.js: $(TH_FILES_RSRC)
tsim.js: EMCCFLAGS_LD += $(TH_FLAGS_RSRC)
tsim.js: EMCCFLAGS_LD += --js-library os.js

%.js: LDFLAGS =# reset LDFLAGS
tcc.bc: CFLAGS =# reset CFLAGS
tcc.bc: CPPFLAGS =# reset CPPFLAGS
tcc.bc: CFLAGS += -Wno-pointer-sign -Wno-sign-compare -fno-strict-aliasing
# clang doesn't know we are using old-style variable-sized structs
tcc.bc: CFLAGS += -Wno-array-bounds
tcc.bc: LDFLAGS += -O2

# compensate for missing search.h implementation in emscripten environment
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX): common.o
tas$(EXE_SUFFIX) tld$(EXE_SUFFIX): tsearch.o lsearch.o
tas$(EXE_SUFFIX): asm.o asmif.o param.o parser.o lexer.o obj.o
tsim$(EXE_SUFFIX): sim.o simif.o param.o asm.o obj.o ram.o serial.o lsearch.o
tld$(EXE_SUFFIX): common.o obj.o

tsim.js: CC_OPT = -O2 # -O0 causes failure

# Disable closure compiler for now
tcc.js tas.js tsim.js tld.js: CLOSURE_FLAGS :=# empty

# Disable closing of streams so that the same code can run again
tas.bc tsim.bc tld.bc: CPPFLAGS += '-Dfclose=fflush'

asm.o ram.o serial.o sparseram.o parser.o lexer.o: CFLAGS += -Wno-unused-parameter
asm.o simif.o: CFLAGS += -Wno-unused-value
parser.o lexer.o: CFLAGS += -Wno-unused -Wno-sign-compare
lexer.o: CFLAGS += -Wno-unneeded-internal-declaration
param.o: CPPFLAGS += -include src/em.h
# flex-generated code needs POSIX source for fileno()
lexer.o: CPPFLAGS += -D_POSIX_SOURCE

clean_FILES += *.bc *.o *.js.mem tsim.js tas.js tld.js tcc.js tcc.data lexer.[ch] parser.[ch] tsim.data

clean clobber::
	$(MAKE) EXE_SUFFIX=.bc -C pp_build $@

