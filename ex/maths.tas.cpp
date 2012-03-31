#include "common.th"

start:
  f <- p - .
  o <- -1

  c <- 11
  d <- 5
  call(umod)

  c <- 50
  d <- 10
  call(udiv)

  c <- 36
  call(isqrt)

  illegal

