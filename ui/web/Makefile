.PHONY: all
BIN_TARGETS = tcc.js tas.js tsim.js
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

%.js: EMCCFLAGS_LD += -s SAFE_HEAP_LINES=0

ifeq ($(DEBUG),)
# --closure 1 probably fails because system functions shared between the
#  multiple separate .js binaries are renamed in different ways. --closure 1
#  works fine as long as only one binary is loaded into the web page.
%.js: EMCCFLAGS_LD += -O2 --closure 0
%.js: EMCCFLAGS_LD += -s ASSERTIONS=0 -s MICRO_OPTS=1 \
                      -s LABEL_DEBUG=0 -s LIBRARY_DEBUG=0 \
                      -s CLOSURE_ANNOTATIONS=1 \
                      -s CHECK_OVERFLOWS=0 \
                      -s CORRECT_OVERFLOWS=0 \
                      -s DISABLE_EXCEPTION_CATCHING=1 \
                      # TODO make relooping work \
                      #-s RELOOP=1 \
                      #
else
%.js: EMCCFLAGS_LD += -O2 # -O0 causes failure
%.js: EMCCFLAGS_LD += -s ASSERTIONS=2 -s MICRO_OPTS=0 \
                      -s LABEL_DEBUG=0 \
                      -s LIBRARY_DEBUG=0 \
                      -s WARN_ON_UNDEFINED_SYMBOLS=1 \
                      #-s CHECK_SIGNS=1 \
                      #-s VERBOSE=1 \
                      #
endif

################################################################################
# Specific rules

# We have to do our own --pre-js equivalent for now, because the --pre-js
# content comes before the main code but after the FS-setup code, and we need
# ours to enclose all modifications to Module.
# We would like to use `-i ''` on MacOS X and `-i` on Linux but they are
# mutually incompatible
HAVE_GNU_SED := $(shell sed --version 2>&1 | grep -c GNU)
ifneq ($(HAVE_GNU_SED),0)
SED_IN_PLACE = sed -i.bak
else
SED_IN_PLACE = sed -i ''
endif
PREFIX_XFORM = "$(SED_IN_PLACE) -e '1r pre_wrap.js' -e '1N; /PRE_RUN_ADDITIONS/r fsify.txt'"
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --js-transform $(PREFIX_XFORM)
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --post-js $*_post.js

CLEANFILES += *_post.js
%_post.js: post_wrap.js
	sed 's/TENYR_BINARY_NAME/"$*"/g' $< > $@.$$$$ && mv $@.$$$$ $@ || rm $@.$$$$

tcc.js tas.js tsim.js tld.js: %.js: pre_wrap.js %_post.js

tas.bc: CFLAGS =# reset CFLAGS

TENYR_LIB_DIR ?= lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --preload-file ,$(foreach m,$(TH_FILES),$m@$(notdir $m)))
tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

%.js: LDFLAGS =# reset LDFLAGS
tcc.bc: CFLAGS =# reset CFLAGS
tcc.bc: CPPFLAGS =# reset CPPFLAGS
tcc.bc: CFLAGS += -Wno-pointer-sign -Wno-sign-compare -fno-strict-aliasing
# clang doesn't know we are using old-style variable-sized structs
tcc.bc: CFLAGS += -Wno-array-bounds
tcc.bc: LDFLAGS += -O2

# compensate for missing search.h implementation in emscripten environment
tas.bc: emtsearch.o emlsearch.o

param.o: CPPFLAGS += -include src/em.h

CLEANFILES += *.bc

##############################################################################

# `make -B EXE_SUFFIX=.exe` to make a command-line version of the em% code
%.exe: em%.o
ifneq ($(MAKE_VERBOSE),)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
else
	@echo "[ LD ] $@"
	@$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endif

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

