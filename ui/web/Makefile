.PHONY: all
BIN_TARGETS = tcc.js tas.js
TARGETS = $(BIN_TARGETS)
# get dependency information from main Makefiles
TOP = main_build
SUPPRESS_BINARY_RULE = 1
EXE_SUFFIX = .bc
DYLIB_SUFFIX = .so
CPPFLAGS += -D"PATH_SEPARATOR_CHAR='/'"
include main_build/Makefile
include pp_build/Makefile
all: $(BIN_TARGETS)

CHECK_PATHS = src src/gen pp_build
VPATH += $(CHECK_PATHS)
INCLUDES += $(CHECK_PATHS)

################################################################################
# General rules

%.bc: CC = emcc
%.bc: CPPFLAGS += -DEMSCRIPTEN

ifeq ($(DEBUG),)
%.js: EMCCFLAGS_LD += -O2
%.js: EMCCFLAGS_LD += -s ASSERTIONS=0 -s MICRO_OPTS=1 \
                      -s LABEL_DEBUG=0 -s LIBRARY_DEBUG=0 \
                      -s CLOSURE_ANNOTATIONS=1 \
                      -s CHECK_OVERFLOWS=0 \
                      -s CORRECT_OVERFLOWS=0 \
else
%.js: EMCCFLAGS_LD += -O0
%.js: EMCCFLAGS_LD += -s ASSERTIONS=1 -s MICRO_OPTS=0 \
                      -s LABEL_DEBUG=0 \
                      -s LIBRARY_DEBUG=0 \
                      -s WARN_ON_UNDEFINED_SYMBOLS=1 \
                      #-s CHECK_SIGNS=1 \
                      #
endif

################################################################################
# Specific rules

tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --js-transform "perl -i -p ./fsify.pl"
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --pre-js $*_pre.js
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --post-js $*_post.js

tas.bc: CFLAGS =# reset CFLAGS

TENYR_LIB_DIR ?= lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --embed-file ,$(TH_FILES))
tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

tcc.bc: CFLAGS =# reset CFLAGS
tcc.bc: CPPFLAGS =# reset CPPFLAGS
tcc.bc: CFLAGS += -g -Wall
tcc.bc: CFLAGS += -Wno-pointer-sign -Wno-sign-compare -fno-strict-aliasing
# clang doesn't know we are using old-style variable-sized structs
tcc.bc: CFLAGS += -Wno-array-bounds
tcc.bc: LDFLAGS += -O2

# compensate for missing search.h implementation in emscripten environment
tas.bc: emtsearch.o emlsearch.o

param.o: CPPFLAGS += -include src/em.h

CLEANFILES += *.bc

##############################################################################

%.bc: em%.o
ifneq ($(MAKE_VERBOSE),)
	emcc $(LDFLAGS) $(EMCCFLAGS_CC) -o $@ $^ $(LDLIBS)
else
	@echo "[ EM-CC ] $@"
	@emcc $(LDFLAGS) $(EMCCFLAGS_CC) -o $@ $^ $(LDLIBS)
endif

%.js: %.bc
ifneq ($(MAKE_VERBOSE),)
	emcc $(LDFLAGS) $(EMCCFLAGS_LD) -o $@ $<
else
	@echo "[ EM-LD ] $@"
	@emcc $(LDFLAGS) $(EMCCFLAGS_LD) -o $@ $<
endif

