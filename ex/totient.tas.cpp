#include "common.th"

_start:
  prologue

  c <- 67
  call(totient)

  illegal

// Computes the number of values less than C that are relatively prime to C.
// Stores the result in B.
totient:
  pushall(d,k)
  b <- 0
  d <- c

loop:
  k <- d == a
  jnzrel(k, done)

  pushall(b,c,d)

  call(gcd)

  k <- b == 1

  popall(b,c,d)

  b <- b - k + 2

  d <- d - 1
  goto(loop)

done:
  popall(d,k)
  ret
