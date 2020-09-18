TOP := ..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

vpath %.tas $(TOP)/lib

%.tas: INCLUDES += $(TOP)/lib

TARGETS = \
    bm_chars.texe       \
    bm_conway.texe      \
    bm_fib.texe         \
    bm_gpio.texe        \
    bm_mults.texe       \
    bm_snake.texe       \
    bsearch_demo.texe   \
    clock.texe          \
    compare.texe        \
    copy.texe           \
    fib.texe            \
    fib_iter.texe       \
    hello.texe          \
    irc.texe            \
    isprime.texe        \
    maths.texe          \
    parse_hex.texe      \
    qsort_demo.texe     \
    set.texe            \
    sieve.texe          \
    test_imul.texe      \
    totient.texe        \
    trailz_demo.texe    \
    #

clean_FILES += $(TARGETS) *.to *.texe

all: $(TARGETS)

hello.texe: puts.to
compare.texe: strcmp.to puts.to
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
test_imul.texe: imul.to puts.to print_hex.to
copy.texe: memcpy.to puts.to

