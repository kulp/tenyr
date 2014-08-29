TOP = $(abspath ..)
.SUFFIXES:
.SUFFIXES: .tas .to .texe

TOOLDIR := $(shell $(MAKE) BUILDDIR= -C $(TOP) showbuilddir)
TAS = $(TOOLDIR)/tas
TLD = $(TOOLDIR)/tld

INCLUDES += -I../lib -I../lib/dword
CPPFLAGS += $(INCLUDES)
CPP := $(CPP) $(DEFINES) -

ECHO := $(shell which echo)

ifeq ($V,1)
SILENCE =
MAKESTEP = true
else
SILENCE = @
MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))
endif

vpath %.tas ../lib ../lib/dword
vpath %.tas.cpp ../lib ../lib/dword

TARGETS = \
    bsearch_demo.texe   \
    bm_snake.texe       \
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

$(patsubst %.tas,%.texe,$(patsubst %.tas.cpp,%.tas,$(wildcard bm_*.tas*))): \
		bm_%.texe: baremetal_common.to

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
bm_snake.texe: rand.to
bm_fib.texe bm_mults.texe: putnum.to
clock.texe: sleep.to

%.tas: %.tas.cpp ../lib/common.th
	@$(MAKESTEP) "[ CPP ] $<"
	$(SILENT)mkdir -p $(*D)
	$(SILENT)$(CPP) $(CPPFLAGS) < $< -o $@

%.to: %.tas
	@$(MAKESTEP) "[ TAS ] $<"
	$(SILENT)mkdir -p $(*D)
	$(SILENT)$(TAS) -o$@ $<

%.texe: %.to
	@$(MAKESTEP) "[ TLD ] $@"
	@mkdir -p $(*D)
	$(SILENT)$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to dword/*.to
	rmdir dword

