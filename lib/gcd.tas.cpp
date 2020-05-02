#include "common.th"

.global gcd

// Computes GCD(C, D) and stores it in B.
// The variant which uses subtraction will ultimately be a lot cheaper than
// the traditional approach involving mod.
gcd:
  push(k)
  // If C == 0, return D.
  b <- d
  k <- c == 0
  p <- @+done & k + p

  b <- c

loop:
  k <- d == 0
  p <- @+done & k + p

  k <- d < b
  p <- @+else & k + p

  d <- d - b
  p <- p + @+loop

else:
  b <- b - d
  p <- p + @+loop

done:
  pop(k)
  ret
