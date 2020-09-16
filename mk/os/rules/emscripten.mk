# Cannot use closure compiler on shared libraries
libtenyr%$(DYLIB_SUFFIX): CLOSURE_FLAGS =# empty
libtenyr%$(DYLIB_SUFFIX): LDFLAGS       = -s SIDE_MODULE=1
preamble.o preamble,dy.o: CFLAGS += -Wno-dollar-in-identifier-extension
# EM_ASM requires GNU mode
preamble.o: CFLAGS += -std=gnu99
# Disable closure compiler for tools using shared libraries
tsim$(EXE_SUFFIX): CLOSURE_FLAGS :=# empty

tsim.js: LDFLAGS += -s MAIN_MODULE=2

# pre.js is needed for node.js support of blocking stdin
$(BIN_TARGETS): $(TOP)/src/os/emscripten/pre.js
# dirname.js is needed for node.js to find memory initialization files if they
# are not in the current directory
$(BIN_TARGETS): $(TOP)/src/os/emscripten/dirname.js
$(BIN_TARGETS): LDFLAGS += --pre-js $(TOP)/src/os/emscripten/pre.js
$(BIN_TARGETS): LDFLAGS += --js-transform "$(TOP)/scripts/inject_file.sh '{{PREAMBLE_ADDITIONS}}' $(TOP)/src/os/emscripten/dirname.js"

$(BIN_TARGETS): %$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) -o $@ $(filter %.o,$^) $(LDLIBS)

# Disable closing of streams so that the same code can run again
$(BIN_TARGETS): CPPFLAGS += '-Dfclose=fflush'
