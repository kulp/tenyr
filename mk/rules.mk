.DEFAULT_GOAL = all

%.tas: %.tas.cpp
	@$(MAKESTEP) "[ TPP ] $(<F)"
	mkdir -p $(*D)
	$(tpp) $(CPPFLAGS) - < $< -o $@

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

OUTPUT_OPTION ?= -o $@
COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
	@$(MAKESTEP) "[ CC ] $(<F)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%,dy.o: CFLAGS += $(CFLAGS_PIC)
%,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(COMPILE.c) -o $@ $<

libtenyr%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(LINK.c) -shared -o $@ $^ $(LDLIBS)

%.vpi: CFLAGS  += $(shell $(IVERILOG)iverilog-vpi --cflags 2> $(DEVNUL) | sed s/-no-cpp-precomp//)
%.vpi: CFLAGS  += -Wno-strict-prototypes
# it's all right for callbacks not to use all their parameters
%.vpi: CFLAGS  += -Wno-unused-parameter
%.vpi: LDFLAGS += $(shell $(IVERILOG)iverilog-vpi --ldflags 2> $(DEVNUL))
%.vpi: LDLIBS  += $(shell $(IVERILOG)iverilog-vpi --ldlibs 2> $(DEVNUL))
%.vpi: %,dy.o
	@$(MAKESTEP) "[ VPI ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	$(LEX) --header-file=$*.h -o $*.c $<
	# Hack around an issue where gcov gets line numbers off by one after the rules section
	-sed /XXXREMOVE/d < $*.c > $*.c.$$$$ && mv $*.c.$$$$ $*.c

%.h %.c: %.y
	@$(MAKESTEP) "[ BISON ] $(<F)"
	$(YACC.y) --defines=$*.h -o $*.c $<

%.d: %.c
	@set -e; mkdir -p $(@D); rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> $(DEVNUL) && \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ && \
	rm -f $@.$$$$ || rm -f $@.$$$$

clean clobber::
	$(RM) -rf $($@_FILES)

clobber:: clean

# vi: set syntax=make: #
