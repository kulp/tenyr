#include "common.th"

.global umod

// Performs C % D and stores the result in B.
umod:
  o <- o - 2
  c -> [o + (2 - 0)]
  d -> [o + (2 - 1)]
  push(p + 2); p <- @+udiv + p
  o <- o + 2
  d <- [o - 1]
  c <- [o - 0]

  b <- b * d
  b <- c - b

  o <- o + 1
  p <- [o]

