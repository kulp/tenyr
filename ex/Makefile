.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib -I../lib/dword

vpath %.tas ../lib ../lib/dword
vpath %.tas.cpp ../lib ../lib/dword

TARGETS = hello.texe fib.texe fib_iter.texe compare.texe sieve.texe maths.texe bsearch.texe irc.texe
CLEANFILES += $(TARGETS)

all: $(TARGETS)

hello.texe: puts.to
compare.texe: strcmp.to puts.to
maths.texe: isqrt.to umod.to udiv.to dword/add.to dword/mul.to
totient.texe: gcd.to
isprime.texe: umod.to isqrt.to udiv.to
bsearch.texe: puts.to
irc.texe: puts.to strcmp.to udiv.to umod.to

%.tas: %.tas.cpp
	mkdir -p $(*D)
	cpp $(CPPFLAGS) $< -o $@

%.to: %.tas
	mkdir -p $(*D)
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to dword/*.to
	rmdir dword

