#include "common.th"

.global dw_add

// Performs B:C = B:C + D:E
dw_add:
  o <- o - 3
  h -> [o + (3 - 0)]
  j -> [o + (3 - 1)]
  k -> [o + (3 - 2)]
  // Computes lower sum.
  h <- c + e

  // Compute carry and store it in K
  j <- c < h
  k <- e < h
  k <- j & k + 1

  // Finish computing sum.
  b <- b + k
  b <- b + d
  c <- h

  o <- o + 4
  k <- [o - (1 + 2)]
  j <- [o - (1 + 1)]
  h <- [o - (1 + 0)]
  p <- [o]

