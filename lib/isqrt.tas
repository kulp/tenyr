#include "common.th"

.global isqrt

// Performs sqrt(C) and stores the result in B.
isqrt:
  b <- 0

  // Compute the highest power of four greater than or equal to n.
  g <- 1
  g <- g << 30
compute_4:
  k <- g <= c
  jnzrel(k, do_magic)
  g <- g >> 2
  goto(compute_4)

  // :)
do_magic:
  k <- g == 0
  jnzrel(k, done)

  e <- b + g
  j <- c <= e
  k <- c <> e
  k <- j & k
  jnzrel(k, shift)

  c <- c - e
  b <- b + g
  b <- b + g

shift:
  b <- b >> 1
  g <- g >> 2
  goto(do_magic)

done:
  ret

