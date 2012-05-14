#include "common.th"

.global srand
.global rand

// Sets the RNG's seed to the value in C.
srand:
  c -> [rel(state)]
  ret

// Returns an unsigned 32-bit random number in B.
rand:
  // Grab the state value.
  b <- [rel(state)]

  // Perform a 32-bit xorshift.
  c <- b << 13
  b <- b ^ c
  c <- b >> 17
  b <- b ^ c
  c <- b << 5
  b <- b ^ c

  // Store the new state.
  b -> [rel(state)]
  ret

state:
  .word 2463534242
