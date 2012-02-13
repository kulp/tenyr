#include "common.th"

.global umod

// Performs C % D and stores the result in B.
umod:
  push(c)
  push(d)

  call(udiv)

  pop(d)
  pop(c)

  b <- b * d
  c <- c - b
  b <- c

  ret

