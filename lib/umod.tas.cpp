#include "common.th"

.global umod

// Performs C % D and stores the result in B.
umod:
  pushall(c,d)
  call(udiv)
  popall(c,d)

  b <- b * d
  b <- c - b

  o <- o + 1
  p <- [o]

