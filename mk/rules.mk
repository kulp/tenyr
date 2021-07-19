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

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	# `sed` here hacks around an issue where gcov gets line numbers off by one
	# after the rules section
	$(LEX) --header-file=$*.h --outfile=$*.c $<

%.h %.c: %.y
	@$(MAKESTEP) "[ BISON ] $(<F)"
	$(YACC.y) --defines=$*.h -o $*.c $<

$(FAILURE_TARGETS): libtenyrfailure%$(DYLIB_SUFFIX): pluginimpl,dy.o $(shared_OBJECTS:%.o=%,dy.o)
failure%,dy.o: CPPFLAGS += -DFAILURE$*=ENOTSUP
failure%,dy.o: CPPFLAGS += -DFAILURE_ADD_DEVICE_FUNC=failure$*_add_device
failure_NO_ADD_DEVICE,dy.o: CPPFLAGS += -UFAILURE_ADD_DEVICE_FUNC -DFAILURE_ADD_DEVICE_FUNC=unfindable_function_name
failure%,dy.o: failure.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

clean clobber::
	$(RM) -rf $($@_FILES)

clobber:: clean

# vi: set syntax=make: #
