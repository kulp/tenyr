#include "common.th"

_start:
  o <- ((1 << 13) - 1)

  c <- 67
  [o] <- p + 2 ; o <- o - 1 ; p <- @+totient + p

  illegal

// Computes the number of values less than C that are relatively prime to C.
// Stores the result in B.
totient:
  o <- o - 2
  d -> [o + (2 - 0)]
  k -> [o + (2 - 1)]
  b <- 0
  d <- c

loop:
  k <- d == a
  p <- @+done & k + p

  o <- o - 3
  b -> [o + (3 - 0)]
  c -> [o + (3 - 1)]
  d -> [o + (3 - 2)]

  [o] <- p + 2 ; o <- o - 1 ; p <- @+gcd + p

  k <- b == 1

  o <- o + 3
  d <- [o - 2]
  c <- [o - 1]
  b <- [o - 0]

  b <- b - k + 2

  d <- d - 1
  p <- p + @+loop

done:
  o <- o + 3
  k <- [o - (1 + 1)]
  d <- [o - (1 + 0)]
  p <- [o]
