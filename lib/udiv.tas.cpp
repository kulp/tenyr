#include "common.th"

.global udiv

// Performs C / D and stores the result in B.
// Assumes C and D are 31-bit unsigned
udiv:
  pushall(d,g,j,k)
  b <- 0

  // Check for division by zero.
  k <- d == 0
  p <- @+done & k + p

  // Check for divisor > dividend.
  k <- c < d
  p <- @+done & k + p

  // Prepare the value to subtract from the dividend.
  g <- d
build_subtrahend:
  // If the value is greater than the dividend, we're done.
  k <- c < g
  p <- @+compute_quotient & k + p

  // Otherwise, multiply by 2.
  g <- g << 1
  p <- p + @+build_subtrahend

compute_quotient:
  // If our subtrahend is equal to our divisor, we're done.
  j <- g <  d
  k <- g == d
  k <- j | k
  p <- @+done & k + p

  // Divide the subtrahend by 2.
  g <- g >> 1

  // Check to see if we can subtract the subtrahend.
  k <- c < g
  p <- @+shift_quotient & k + p

  // Perform a subtraction and set this output bit to 1.
  c <- c - g
  b <- b | 1

  // Each iteration, shift the quotient left by one.
shift_quotient:
  b <- b << 1
  p <- p + @+compute_quotient

  // Return result.
done:
  b <- b >> 1
  popall(d,g,j,k)
  ret

