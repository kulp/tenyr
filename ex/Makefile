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
dynamic.texe: memset.to malloc.to abort.to
maths.texe: isqrt.to umod.to udiv.to dword/add.to dword/dword_mul.to
totient.texe: gcd.to
isprime.texe: umod.to isqrt.to udiv.to
bsearch_demo.texe: puts.to
qsort_demo.texe: qsort.to puts.to memcpy.to
irc.texe: puts.to strcmp.to udiv.to umod.to strtol.to
parse_hex.texe: strtol.to

%.tas: %.tas.cpp
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(CPP) $(CPPFLAGS) < $< -o $@
else
	@echo "[ CPP ] $<"
	@mkdir -p $(*D)
	@$(CPP) $(CPPFLAGS) < $< -o $@
endif

%.to: %.tas
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(TAS) -o$@ $<
else
	@echo "[ TAS ] $<"
	@mkdir -p $(*D)
	@$(TAS) -o$@ $<
endif

%.texe: %.to
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(TLD) -o$@ $^
else
	@echo "[ TLD ] $@"
	@mkdir -p $(*D)
	@$(TLD) -o$@ $^
endif

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to dword/*.to
	rmdir dword

