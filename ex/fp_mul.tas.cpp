#include "common.th"

.global fp_mul

// Performs B <- C * D, where B, C, & D are binary32 floating-point values.
fp_mul:
  // Compute the sign.
  b <- c >> 31
  e <- d >> 31
  b <- b ^ e
  b <- b << 31

  // Compute the exponent.
  e <- c >> 23
  e <- e & 0xff
  g <- d >> 23
  g <- g & 0xff
  e <- e - 0x7f + g

  // Extract the mantissas.
  m <- [@+mant + p]
  c <- c & m
  d <- d & m

  // Add the implicit bit to the mantissas.
  m <- 1
  m <- m << 23
  c <- c | m
  d <- d | m

  // Save what we have so far.
  push(b)
  push(e)

  e <- c

  // Multiply the two mantissas together.
  push(p + 2); p <- @+dw_mul + p

  // B:C contains a 48-bit product, take the upper 23 bits.
  b <- b << 9
  c <- c >>> 23 + b

  // Check to see if the exponent needs adjustment.
  pop(e)
  m <- 1
  m <- m << 23
  m <- m & c
  m <- 0 < m
  p <- @+no_shift & m + p

  // Adjust exponent.
  e <- e + 1
  c <- c >>> 1

no_shift:
  // Restore the saved sign and combine the result.
  pop(b)
  b <- e << 23 + b
  m <- [@+mant + p]
  c <- c & m
  b <- b | c

  o <- o + 1
  p <- [o]

mant: .word 0x7fffff
