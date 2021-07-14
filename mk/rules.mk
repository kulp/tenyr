.DEFAULT_GOAL = all

%.to: %.tas
	@$(MAKESTEP) "[ TAS ] $(<F)"
	mkdir -p $(*D)
	$(tas) -o$@ $<

%.texe: %.to
	@$(MAKESTEP) "[ TLD ] $(@F)"
	@mkdir -p $(*D)
	$(tld) -o$@ $^

%.memh: %.texe
	@$(MAKESTEP) "[ MEMH ] $(<F)"
	$(tas) -vd $< | $(tas) -fmemh -o $@ -

# Set up dependency generation flags.
%.o: CPPFLAGS += -MMD -MT '$*.o $*,dy.o $*.d' -MF $*.d

%.o: %.c
	@$(MAKESTEP) "[ CC ] $(<F)"
	$(COMPILE.c) -o $@ $<

%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

%,dy.o: CFLAGS += $(CFLAGS_PIC)
%,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

libtenyr%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(LINK.c) -shared -o $@ $^ $(LDLIBS)

%.vpi: CFLAGS  += $(shell $(IVERILOG)iverilog-vpi --cflags 2> /dev/null | sed s/-no-cpp-precomp//)
%.vpi: CFLAGS  += -Wno-strict-prototypes
# it's all right for callbacks not to use all their parameters
%.vpi: CFLAGS  += -Wno-unused-parameter
%.vpi: LDFLAGS += $(shell $(IVERILOG)iverilog-vpi --ldflags 2> /dev/null)
%.vpi: LDLIBS  += $(shell $(IVERILOG)iverilog-vpi --ldlibs 2> /dev/null)
%.vpi: %,dy.o
	@$(MAKESTEP) "[ VPI ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	# `sed` here hacks around an issue where gcov gets line numbers off by one
	# after the rules section
	$(LEX) --header-file=$*.h --stdout $< | sed /XXXREMOVE/d > $*.c

%.h %.c: %.y
	@$(MAKESTEP) "[ BISON ] $(<F)"
	$(YACC.y) --defines=$*.h -o $*.c $<

$(FAILURE_TARGETS): libtenyrfailure%$(DYLIB_SUFFIX): pluginimpl,dy.o $(shared_OBJECTS:%.o=%,dy.o)
failure%,dy.o: CPPFLAGS += -DFAILURE$*=ENOTSUP
failure%,dy.o: CPPFLAGS += -DFAILURE_ADD_DEVICE_FUNC=failure$*_add_device
failure%,dy.o: failure.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

clean clobber::
	$(RM) -rf $($@_FILES)

clobber:: clean

# vi: set syntax=make: #
