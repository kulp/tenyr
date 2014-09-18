makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
HERE := $(dir $(makefile_path))
TOP ?= $(HERE)/main_build
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

BIN_TARGETS = tcc.js tas.js tsim.js
TARGETS = $(BIN_TARGETS)
EXE_SUFFIX = .bc
CPPFLAGS += -D"PATH_SEPARATOR_CHAR='/'"

.PHONY: all
all:: $(BIN_TARGETS)

MAIN_BUILD_DIR := $(shell $(MAKE) -s -C main_build showbuilddir)

CHECK_PATHS = src main_build/src $(MAIN_BUILD_DIR) main_build/src/os/Linux
vpath %.h $(CHECK_PATHS)
vpath %.c $(CHECK_PATHS) main_build/src/devices
vpath %.bc pp_build
INCLUDES += $(CHECK_PATHS)

EMCC = emcc

################################################################################
# General rules

%.bc: export CC = $(EMCC)
%.bc: export EXE_SUFFIX = .bc
%.bc: CPPFLAGS += -DEMSCRIPTEN
%.bc: CPPFLAGS += -Wno-warn-absolute-paths

%.js: EMCCFLAGS_LD += -s SAFE_HEAP_LINES=0

ifeq ($(DEBUG),)
CC_OPT = -O2
CC_DEBUG =
# --closure 1 probably fails because system functions shared between the
#  multiple separate .js binaries are renamed in different ways. --closure 1
#  works fine as long as only one binary is loaded into the web page.
%.js: EMCCFLAGS_LD += $(CC_OPT) $(CC_DEBUG) --closure 0
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
CC_OPT = -O0
CC_DEBUG = -g
%.js: EMCCFLAGS_LD += $(CC_OPT) $(CC_DEBUG)
%.js: EMCCFLAGS_LD += -s ASSERTIONS=2 -s MICRO_OPTS=0 \
                      -s LABEL_DEBUG=0 \
                      -s LIBRARY_DEBUG=0 \
                      -s WARN_ON_UNDEFINED_SYMBOLS=1 \
                      #-s CHECK_SIGNS=1 \
                      #-s VERBOSE=1 \
                      #
endif

# TODO obviate this hack
%.js: EMCCFLAGS_LD += --memory-init-file 0

%$(EXE_SUFFIX): em%.o
	@$(MAKESTEP) "[ LD ] $@"
	$(SILENCE)$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.js: %.bc
	@$(MAKESTEP) "[ EM-LD ] $@"
	$(SILENCE)$(EMCC) $(EMCCFLAGS_LD) $< $(LDLIBS) -o $@

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
PREFIX_XFORM = "$(SED_IN_PLACE) -e '1r pre_wrap.js' -e '1N; /{{PRE_RUN_ADDITIONS}}/r fsify.txt'"
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --js-transform $(PREFIX_XFORM)
tcc.js tas.js tsim.js tld.js: EMCCFLAGS_LD += --post-js $*_post.js

clean_FILES += *_post.js
%_post.js: post_wrap.js
	sed 's/TENYR_BINARY_NAME/"$*"/g' $< > $@.$$$$ && mv $@.$$$$ $@ || rm $@.$$$$

tcc.js tas.js tsim.js tld.js: %.js: pre_wrap.js %_post.js

TENYR_LIB_DIR ?= lib
TH_FILES = $(wildcard $(TENYR_LIB_DIR)/*.th)
TH_FLAGS = $(addprefix --preload-file ,$(foreach m,$(TH_FILES),$m@$(notdir $m)))
tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

tcc.bc:
	$(MAKE) -C pp_build DEBUG=$(CC_DEBUG) CC_OPT=$(CC_OPT) $@
	cp pp_build/$(@F) $@

# compensate for missing search.h implementation in emscripten environment
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX): common.o
tas$(EXE_SUFFIX) tld$(EXE_SUFFIX): emtsearch.o emlsearch.o
tas$(EXE_SUFFIX): asm.o asmif.o parser.o lexer.o obj.o
tsim$(EXE_SUFFIX): sim.o simif.o param.o asm.o obj.o ram.o serial.o debugwrap.o
tld$(EXE_SUFFIX): common.o obj.o

tsim.js: CC_OPT = -O2 # -O0 causes failure

asm.o debugwrap.o ram.o serial.o sparseram.o parser.o lexer.o: CFLAGS += -Wno-unused-parameter
asm.o simif.o: CFLAGS += -Wno-unused-value
parser.o lexer.o: CFLAGS += -Wno-unused -Wno-sign-compare
lexer.o: CFLAGS += -Wno-unneeded-internal-declaration
param.o: CPPFLAGS += -include src/em.h

clean_FILES += *.bc *.o *.js.mem tsim.js tas.js tld.js tcc.js tcc.data

clean clobber::
	$(MAKE) EXE_SUFFIX=.bc -C pp_build $@

