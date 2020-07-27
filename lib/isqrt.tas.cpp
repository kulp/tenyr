#include "common.th"

.global isqrt

// Performs sqrt(C) and stores the result in B.
isqrt:
  o <- o - 4
  e -> [o + (4 - 0)]
  g -> [o + (4 - 1)]
  j -> [o + (4 - 2)]
  k -> [o + (4 - 3)]
  b <- 0

  // Compute the highest power of four greater than or equal to n.
  g <- 1
  g <- g << 30
compute_4:
  j <- g <  c
  k <- g == c
  k <- j | k
  p <- @+do_magic & k + p
  g <- g >> 2
  p <- p + @+compute_4

  // :)
do_magic:
  k <- g == 0
  p <- @+done & k + p

  e <- b + g
  k <- c < e
  p <- @+shift & k + p

  c <- c - e
  b <- g * 2 + b

shift:
  b <- b >> 1
  g <- g >> 2
  p <- p + @+do_magic

done:
  o <- o + 5
  k <- [o - (1 + 3)]
  j <- [o - (1 + 2)]
  g <- [o - (1 + 1)]
  e <- [o - (1 + 0)]
  p <- [o]

