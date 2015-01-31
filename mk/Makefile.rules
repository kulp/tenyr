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

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
%$(EXE_SUFFIX): %.o
	@$(MAKESTEP) "[ LD ] $@"
	$(SILENCE)$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.h %.c: %.l
	@$(MAKESTEP) "[ FLEX ] $(<F)"
	$(SILENCE)$(FLEX) --header-file=$*.h -o $*.c $<

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
