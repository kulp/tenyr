#include "common.th"

.global isqrt

// Performs sqrt(C) and stores the result in B.
isqrt:
  pushall(e,g,j,k)
  b <- 0

  // Compute the highest power of four greater than or equal to n.
  g <- 1
  g <- g << 30
compute_4:
  j <- g <  c
  k <- g == c
  k <- j | k
  jnzrel(k, do_magic)
  g <- g >> 2
  p <- p + @+compute_4

  // :)
do_magic:
  k <- g == 0
  jnzrel(k, done)

  e <- b + g
  k <- c < e
  jnzrel(k, shift)

  c <- c - e
  b <- g * 2 + b

shift:
  b <- b >> 1
  g <- g >> 2
  p <- p + @+do_magic

done:
  popall(e,g,j,k)
  ret

