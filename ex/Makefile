.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

INCLUDES += -I../lib -I../lib/dword
CPPFLAGS += -xc $(INCLUDES)
CPP := $(CPP) -

vpath %.tas ../lib ../lib/dword
vpath %.tas.cpp ../lib ../lib/dword

TARGETS = \
    bsearch_demo.texe   \
    compare.texe        \
    fib.texe            \
    fib_iter.texe       \
    hello.texe          \
    isprime.texe        \
    irc.texe            \
    maths.texe          \
    parse_hex.texe      \
    qsort_demo.texe     \
    sieve.texe          \
    #

CLEANFILES += $(TARGETS)

all: $(TARGETS)

hello.texe: puts.to
compare.texe: strcmp.to puts.to
dynamic.texe: memset.to malloc.to
maths.texe: isqrt.to umod.to udiv.to dword/add.to dword/mul.to
totient.texe: gcd.to
isprime.texe: umod.to isqrt.to udiv.to
bsearch_demo.texe: puts.to
qsort_demo.texe: qsort.to puts.to memcpy.to
irc.texe: puts.to strcmp.to udiv.to umod.to strtol.to
parse_hex.texe: strtol.to

%.tas: %.tas.cpp
	mkdir -p $(*D)
	$(CPP) $(CPPFLAGS) < $< > $@

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

