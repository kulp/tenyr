# Cannot use closure compiler on shared libraries
$(LIB_TARGETS): CLOSURE_FLAGS =# empty
$(LIB_TARGETS): LDFLAGS       = -s SIDE_MODULE=1
preamble.o preamble,dy.o: CFLAGS += -Wno-dollar-in-identifier-extension
# EM_ASM requires advanced (and GNU-specific) features
preamble.o: CFLAGS += -std=gnu11
# Disable closure compiler for tools using shared libraries
tsim$(EXE_SUFFIX): CLOSURE_FLAGS :=# empty

CPPFLAGS += -Wno-error=fastcomp

# For now, avoid failing on emscripten warnings (which can be as innocuous as
# "disabling closure because debug info was requested"
CFLAGS += -Wno-error=emcc

# Avoid erroring on tricks used by EM_ASM
CFLAGS += -Wno-gnu-zero-variadic-macro-arguments
CFLAGS += -Wno-c11-extensions

# Wasm compilation needs PIC
CFLAGS += -fPIC
LDFLAGS += -fPIC

tcc.js: LDFLAGS += $(TH_FLAGS)

tsim.js: $(RSRC_FILES)
tsim.js: LDFLAGS += $(RSRC_FLAGS)
tsim.js: LDFLAGS += -s MAIN_MODULE=2

vpath %.c $(PP_BUILD)
vpath %.h $(PP_BUILD)
tcc.o: libtcc.c tccpp.c tccgen.c tcc.h libtcc.h tcctok.h
tcc$(EXE_SUFFIX): CFLAGS := $(CC_DEBUG) -Wall
tcc$(EXE_SUFFIX): CFLAGS += -Wno-pointer-sign -Wno-sign-compare -fno-strict-aliasing -Wno-shift-negative-value
tcc$(EXE_SUFFIX): CFLAGS += $(CC_OPT)

# pre.js is needed for node.js support of blocking stdin
$(BIN_TARGETS): $(TOP)/ui/web/pre.js
# dirname.js is needed for node.js to find memory initialization files if they
# are not in the current directory
$(BIN_TARGETS): $(TOP)/ui/web/dirname.js
$(BIN_TARGETS): LDFLAGS += --pre-js $(TOP)/ui/web/pre.js
$(BIN_TARGETS): LDFLAGS += --js-transform "$(TOP)/ui/web/inject_file.sh '{{PREAMBLE_ADDITIONS}}' $(TOP)/ui/web/dirname.js"

$(BIN_TARGETS): %$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) -o $@ $(filter %.o,$^) $(LDLIBS)
	echo "var INHIBIT_RUN; if (!INHIBIT_RUN) Module_$*();" >> $@

# Disable closing of streams so that the same code can run again
$(BIN_TARGETS): CPPFLAGS += '-Dfclose=fflush'
