.DEFAULT_GOAL = all

%.tas: %.tas.cpp
	@$(MAKESTEP) "[ TPP ] $(<F)"
	mkdir -p $(*D)
	$(TPP) $(CPPFLAGS) - < $< -o $@

%.to: %.tas
	@$(MAKESTEP) "[ TAS ] $(<F)"
	mkdir -p $(*D)
	$(TAS) -o$@ $<

%.texe: %.to
	@$(MAKESTEP) "[ TLD ] $(@F)"
	@mkdir -p $(*D)
	$(TLD) -o$@ $^

%.memh: %.texe
	@$(MAKESTEP) "[ MEMH ] $(<F)"
	$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

OUTPUT_OPTION ?= -o $@
COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
	@$(MAKESTEP) "[ CC ] $(<F)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

%.o: %.cpp
	@$(MAKESTEP) "[ CXX ] $(<F)"
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.cc
	@$(MAKESTEP) "[ CXX ] $(<F)"
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libtenyr%$(DYLIB_SUFFIX): %.cc
	@$(MAKESTEP) "[ DYLD-CXX ] $@"
	$(LINK.cc) -o $@ $^ $(LDLIBS)

%,dy.o: CXXFLAGS += $(CFLAGS_PIC)
%,dy.o: CFLAGS += $(CFLAGS_PIC)
%,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

libtenyr%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(LINK.c) -shared -o $@ $^ $(LDLIBS)

%.vpi: CFLAGS  += $(shell $(IVERILOG)iverilog-vpi --cflags 2> /dev/null | sed s/-no-cpp-precomp//)
%.vpi: CFLAGS  += -Wno-strict-prototypes
# don't complain about unused values that we might use in asserts
# it's all right for callbacks not to use all their parameters
%.vpi: CFLAGS  += -Wno-unused-value -Wno-unused-parameter
%.vpi: LDFLAGS += $(shell $(IVERILOG)iverilog-vpi --ldflags 2> /dev/null)
%.vpi: LDLIBS  += $(shell $(IVERILOG)iverilog-vpi --ldlibs 2> /dev/null)
%.vpi: %,dy.o
	@$(MAKESTEP) "[ VPI ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	$(FLEX) --header-file=$*.h -o $*.c $<
	# Hack around an issue where gcov gets line numbers off by one after the rules section
	-sed /XXXREMOVE/d < $*.c > $*.c.$$$$ && mv $*.c.$$$$ $*.c

%.h %.c: %.y
	@$(MAKESTEP) "[ BISON ] $(<F)"
	$(BISON) --defines=$*.h -o $*.c $<

%.d: %.c
	@set -e; mkdir -p $(@D); rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null && \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ && \
	rm -f $@.$$$$ || rm -f $@.$$$$

clean clobber::
	$(RM) -rf $($@_FILES)

clobber:: clean

# vi: set syntax=make: #
