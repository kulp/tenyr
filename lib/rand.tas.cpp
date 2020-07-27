// Remember not to use the bottom bits of the result for randomness : the
// period is much shorter than the overall PRNG period !

#include "common.th"

// From MTH$RANDOM
// http://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
// Use this one because it saves instructions and a memory load for the
// increment, since we can encode the increment as an immediate.
mult: .word 69069
#define INCREMENT 1

// Here is another possibility
//mult: .word 1664525 // INMOS Transputer
//#define INCREMENT 0

// base the seed on where we get linked in (better than a fixed seed ?)
seed: .word @mult

// RAND_MAX = (2^32 - 1)

.global srand
srand:
    c -> [@+seed + p]
    o <- o + 1
    p <- [o]

.global rand
rand:
    push(c)
    b <- [@+seed + p]
    c <- [@+mult + p]
    b <- b * c + INCREMENT
    b -> [@+seed + p]
    o <- o + 2
    c <- [o - (1 + 0)]
    p <- [o]

