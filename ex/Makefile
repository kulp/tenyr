TOP := $(abspath ..)
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

vpath %.tas $(TOP)/lib
vpath %.tas.cpp $(TOP)/lib

%.tas: INCLUDES += $(TOP)/lib

TARGETS = \
    bsearch_demo.texe   \
    bm_chars.texe       \
    bm_conway.texe      \
    bm_snake.texe       \
    bm_mults.texe       \
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

CPP_FILES := $(wildcard *.tas.cpp)

clean_FILES += $(TARGETS) *.to *.texe
clean_FILES += $(CPP_FILES:%.tas.cpp=%.tas)

all: $(TARGETS)

hello.texe: puts.to
compare.texe: strcmp.to puts.to
dynamic.texe: memset.to malloc.to abort.to
maths.texe: isqrt.to umod.to udiv.to dword_add.to dword_mul.to
totient.texe: gcd.to
isprime.texe: umod.to isqrt.to udiv.to
bsearch_demo.texe: bsearch.to puts.to
qsort_demo.texe: qsort.to puts.to memcpy.to
irc.texe: puts.to strcmp.to udiv.to umod.to strtol.to
parse_hex.texe: strtol.to
bm_snake.texe bm_conway.texe: rand.to
bm_fib.texe bm_mults.texe: putnum.to
clock.texe: sleep.to
trailz_demo.texe: puts.to trailz.to
bm_conway.texe: bm_conway_buffer.to

$(patsubst %.tas.cpp,%.tas,$(CPP_FILES)): %.tas: $(TOP)/lib/common.th

