.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

CPP := $(CPP) -

GOOD = \
	ambiguous.to \
	arrows.to \
	badarrow.to \
	binnum.to \
	charlit.to \
	comments.to \
	data.to \
	div.to \
	exports.to \
	extern.to \
	global.to \
	local.to \
	loop.to \
	negnull.to \
	nl.to \
	precedence.to \
	strings.to \
	twopass.to \
	type1.to \
	underscore.to \
	utf32.to \
	zero.to \
	zeros.to \
	#

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

ASSEMBLE_FAIL = \
	dup.to \
	toolong.to \
	#

CLEANFILES = *.to *.texe

all: positive negative
positive: $(GOOD)
negative: $(ASSEMBLE_FAIL)

test_imul.texe: imul.to puts.to print_hex.to
copy.texe: memcpy.to puts.to

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) < $< > $@

%.to: %.tas
	$(TAS) -o$@ $<

$(filter %.to,$(ASSEMBLE_FAIL)): %.to: %.tas
	@echo "Assembling $@ (should fail) ..."
	@! $(TAS) -o$@ $< 2>/dev/null

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

