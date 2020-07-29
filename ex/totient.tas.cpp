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
  p <- @+done & k + p

  pushall(b,c,d)

  call(gcd)

  k <- b == 1

  popall(b,c,d)

  b <- b - k + 2

  d <- d - 1
  p <- p + @+loop

done:
  popall_ret(d,k)
