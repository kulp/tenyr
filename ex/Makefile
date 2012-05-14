.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib -I../lib/dword

vpath %.tas ../lib ../lib/dword

TARGETS = hello.texe fib.texe fib_iter.texe compare.texe sieve.texe maths.texe
CLEANFILES += $(TARGETS)

all: $(TARGETS)

hello.texe: puts.to
compare.texe: strcmp.to puts.to
maths.texe: isqrt.to umod.to udiv.to dword/add.to dword/mul.to

%.tas: %.tas.cpp
	cpp $(CPPFLAGS) $< -o $@

%.to: %.tas
	mkdir -p $(dir $@)
	$(TAS) -o$@ $< || (rm $@ ; exit 1)

%.texe: %.to
	$(TLD) -o$@ $^ || (rm $@ ; exit 1)

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to dword/*.to
	rmdir dword

