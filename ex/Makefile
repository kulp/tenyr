TOP := $(abspath ..)
include $(TOP)/Makefile.common
include $(TOP)/Makefile.rules

.SUFFIXES:
.SUFFIXES: .tas .to .texe

INCLUDES += -I../lib -I../lib/dword
CPPFLAGS += $(INCLUDES)

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

clean_FILES += $(TARGETS) *.to dword/*.to

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

$(patsubst %.tas.cpp,%.tas,$(wildcard *.tas.cpp)): %.tas: $(TOP)/lib/common.th

