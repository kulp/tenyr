#include "common.th"

.global dw_mul

// Performs B:C <- D * E
// Assumes D and E are 31-bit unsigned
dw_mul:
  push(m)
  // Load mask.
  m <- [@+mask + p]

  // Split up factors into 16-bit components.
  b <- d >>> 16
  c <- d & m
  d <- e >>> 16
  e <- e & m

  // Compute upper and lower sums as well as an intermediate component.
  m <- b * e
  b <- b * d
  d <- c * d
  m <- m + d
  c <- c * e

  // Adjust lower sum.
  c <- m << 16 + c

  // Adjust upper sum.
  b <- m >>> 16 + b

  o <- o + 2
  m <- [o - (1 + 0)]
  p <- [o]

mask: .word 0xffff
