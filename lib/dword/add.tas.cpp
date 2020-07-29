#include "common.th"

.global dw_add

// Performs B:C = B:C + D:E
dw_add:
  pushall(h,j,k)
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

  popall_ret(h,j,k)

