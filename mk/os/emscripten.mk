# this file is included by the main Makefile automatically
EXE_SUFFIX = .bc
#CFLAGS_PIC =
EMCC = emcc
CC := $(EMCC)
JIT = 0

BIN_TARGETS := $(BIN_TARGETS:$(EXE_SUFFIX)=.js)

#runwrap := node $(runwrap)

%.bc: USE_OWN_SEARCH = 1

%.bc: export CC = $(EMCC)
%.bc: export EXE_SUFFIX = .bc
%.bc: CPPFLAGS += -DEMSCRIPTEN
%.bc: CPPFLAGS += -Wno-warn-absolute-paths

EMCCFLAGS_LD += -s MODULARIZE=1

SDL_OPTS = \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s 'SDL2_IMAGE_FORMATS=["png"]' \
	#

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

%.js: %.bc
	@$(MAKESTEP) "[ EM-LD ] $@"
	$(EMCC) $(EMCCFLAGS_LD) $< $(LDLIBS) -o $@

# Disable closure compiler for now
tcc.js tas.js tsim.js tld.js: CLOSURE_FLAGS :=# empty

# Disable closing of streams so that the same code can run again
tas.bc tsim.bc tld.bc: CPPFLAGS += '-Dfclose=fflush'

clean_FILES += *.bc *.js.mem tsim.js tas.js tld.js tcc.js tcc.data tsim.data

