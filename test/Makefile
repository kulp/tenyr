.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

CPP := $(CPP) -

get_obj_name = $(patsubst %.tas,%.to,$(patsubst %.tas.cpp,%.tas,$1))

GOOD = $(call get_obj_name,$(wildcard pass_compile/*))

# The contents of $(CHECK) need to be categorised into $(GOOD) and
# $(ASSEMBLE_FAIL). Most of them should probably succeed.
CHECK = \
	break.to \
	emit.to \
	echo.to \
	dots.to \
	error.to \
	imm.to \
	label.to \
	linked.to \
	lowercase.to \
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

ASSEMBLE_FAIL = $(call get_obj_name,$(wildcard fail_compile/*))

CLEANFILES = *.to *.texe

all: positive negative
positive: $(GOOD)
negative: $(ASSEMBLE_FAIL)

test_imul.texe: imul.to puts.to print_hex.to
copy.texe: memcpy.to puts.to

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) < $< > $@

%.to: %.tas
	$(TAS) -o$(OBJDIR)$(@F) $<

$(filter %.to,$(ASSEMBLE_FAIL)): %.to: %.tas
	@echo "Assembling $@ (should fail) ..."
	@! $(TAS) -o$(OBJDIR)$(@F) $< 2>/dev/null

%.texe: %.to
	$(TLD) -o$(OBJDIR)$(@F) $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

