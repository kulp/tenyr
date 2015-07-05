.DEFAULT_GOAL = all

%.tas: %.tas.cpp
	@$(MAKESTEP) "[ CPP ] $(<F)"
	$(SILENCE)mkdir -p $(*D)
	$(SILENCE)$(CPP) $(CPPFLAGS) - < $< -o $@

%.to: %.tas
	@$(MAKESTEP) "[ TAS ] $(<F)"
	$(SILENCE)mkdir -p $(*D)
	$(SILENCE)$(TAS) -o$@ $<

%.texe: %.to
	@$(MAKESTEP) "[ TLD ] $(@F)"
	@mkdir -p $(*D)
	$(SILENCE)$(TLD) -o$@ $^

%.memh: %.texe
	@$(MAKESTEP) "[ MEMH ] $(<F)"
	$(SILENCE)$(TAS) -vd $< | $(TAS) -fmemh -o $@ -

OUTPUT_OPTION ?= -o $@
COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
	@$(MAKESTEP) "[ CC ] $(<F)"
	$(SILENCE)$(COMPILE.c) $(OUTPUT_OPTION) $<

%.o: %.cpp
	@$(MAKESTEP) "[ CXX ] $(<F)"
	$(SILENCE)$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.cc
	@$(MAKESTEP) "[ CXX ] $(<F)"
	$(SILENCE)$(COMPILE.cc) $(OUTPUT_OPTION) $<

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(SILENCE)$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libtenyr%$(DYLIB_SUFFIX): %.cc
	@$(MAKESTEP) "[ DYLD-CXX ] $@"
	$(SILENCE)$(LINK.cc) -o $@ $^ $(LDLIBS)

%,dy.o: CXXFLAGS += $(CFLAGS_PIC)
%,dy.o: CFLAGS += $(CFLAGS_PIC)
%,dy.o: %.c
	@$(MAKESTEP) "[ DYCC ] $(<F)"
	$(SILENCE)$(COMPILE.c) -o $@ $<

libtenyr%$(DYLIB_SUFFIX): %,dy.o
	@$(MAKESTEP) "[ DYLD ] $@"
	$(SILENCE)$(LINK.c) -shared -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	$(SILENCE)$(FLEX) --header-file=$*.h -o $*.c $<
	$(SILENCE)# Hack around an issue where gcov gets line numbers off by one after the rules section
	$(SILENCE)-sed /XXXREMOVE/d < $*.c > $*.c.$$$$ && mv $*.c.$$$$ $*.c

%.h %.c: %.y
	@$(MAKESTEP) "[ BISON ] $(<F)"
	$(SILENCE)$(BISON) --defines=$*.h -o $*.c $<

%.d: %.c
	@set -e; mkdir -p $(@D); rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null && \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ && \
	rm -f $@.$$$$ || rm -f $@.$$$$

clean clobber::
	$(RM) -rf $($@_FILES)

clobber:: clean

# vi: set syntax=make: #
