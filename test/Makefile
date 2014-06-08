.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

vpath %.tas pass_compile/ fail_compile/

CPP := $(CPP) -

get_obj_name = $(notdir $(patsubst %.tas,%.to,$(patsubst %.tas.cpp,%.tas,$1)))

PASS_ASSEMBLE = $(call get_obj_name,$(wildcard pass_compile/*))

# The contents of $(CHECK) need to be categorised into $(PASS_ASSEMBLE) and
# $(FAIL_ASSEMBLE). Most of them should probably succeed.
CHECK = \
	break.to \
	linked.to \
	multiword.to \
	nest.to \
	reloc.to \
	set.to \
	symbols.to \
	unary.to \
	\
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
	@echo "Assembling $@ (should fail) ..."
	@! $(TAS) -o$@ $< 2>/dev/null

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

