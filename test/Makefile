TOP = $(abspath ..)
.SUFFIXES:
.SUFFIXES: .tas .to .texe

TOOLDIR := $(shell $(MAKE) BUILDDIR= -C $(TOP) showbuilddir)
TAS = $(TOOLDIR)/tas
TLD = $(TOOLDIR)/tld

CPPFLAGS += -I../lib

ECHO := $(shell which echo)
MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))

vpath %.tas ../lib
vpath %.tas.cpp ../lib

vpath %.tas pass_compile/ fail_compile/

CPP := $(CPP) -

get_obj_name = $(notdir $(patsubst %.tas,%.to,$(patsubst %.tas.cpp,%.tas,$1)))

# TODO run .texe's instead of just compiling them
PASS_ASSEMBLE = \
	$(call get_obj_name,$(wildcard pass_compile/*)) \
	copy.texe \
	jzrel.texe \
	test_imul.texe \
	#

FAIL_ASSEMBLE = $(call get_obj_name,$(wildcard fail_compile/*))

CLEANFILES = *.to *.texe

all: positive negative
positive: $(PASS_ASSEMBLE)
negative: $(FAIL_ASSEMBLE)

test_imul.texe: imul.to puts.to print_hex.to
copy.texe: memcpy.to puts.to

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) < $< > $@

%.to: %.tas
	$(TAS) -o$@ $<

$(filter %.to,$(FAIL_ASSEMBLE)): %.to: %.tas
	@$(ECHO) -n "Assembling $@ (should fail) ... "
	@(! $(TAS) -o$@ $< 2>/dev/null) && $(ECHO) "failed as expected"

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

