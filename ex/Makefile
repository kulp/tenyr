.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

INCLUDES += -I../lib -I../lib/dword
CPPFLAGS += $(INCLUDES)
CPP := $(CPP) $(DEFINES) -

ECHO := $(shell which echo)
MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))

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
    snake.texe          \
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
bm_mults.texe bm_fib.texe baremetal.texe: putnum.to baremetal_common.to
snake.texe: rand.to baremetal_common.to

%.tas: %.tas.cpp ../lib/common.th
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(CPP) $(CPPFLAGS) < $< -o $@
else
	@$(MAKESTEP) "[ CPP ] $<"
	@mkdir -p $(*D)
	@$(CPP) $(CPPFLAGS) < $< -o $@
endif

%.to: %.tas
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(TAS) -o$@ $<
else
	@$(MAKESTEP) "[ TAS ] $<"
	@mkdir -p $(*D)
	@$(TAS) -o$@ $<
endif

%.texe: %.to
ifneq ($(MAKE_VERBOSE),)
	@mkdir -p $(*D)
	$(TLD) -o$@ $^
else
	@$(MAKESTEP) "[ TLD ] $@"
	@mkdir -p $(*D)
	@$(TLD) -o$@ $^
endif

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to dword/*.to
	rmdir dword

