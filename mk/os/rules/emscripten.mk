# Cannot use closure compiler on shared libraries
$(LIB_TARGETS): CLOSURE_FLAGS =# empty
$(LIB_TARGETS): LDFLAGS       = -s SIDE_MODULE=1
preamble.o preamble,dy.o: CFLAGS += -Wno-dollar-in-identifier-extension
# Disable closure compiler for tools using shared libraries
tsim$(EXE_SUFFIX): CLOSURE_FLAGS :=# empty

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

$(BIN_TARGETS): LDFLAGS += --pre-js $(TOP)/ui/web/pre.js

$(BIN_TARGETS): %$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) -o $@ $(filter %.o,$^) $(LDLIBS)
	echo "var INHIBIT_RUN; if (!INHIBIT_RUN) Module_$*();" >> $@

# Disable closure compiler until it works
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): CLOSURE_FLAGS :=# empty

# Disable closing of streams so that the same code can run again
$(BIN_TARGETS): CPPFLAGS += '-Dfclose=fflush'
