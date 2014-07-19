#include "common.th"

.global udiv

// Performs C / D and stores the result in B.
// Assumes C and D are 31-bit unsigned
udiv:
  pushall(d,g,j,k)
  b <- 0

  // Check for division by zero.
  k <- d == 0
  jnzrel(k, done)

  // Check for divisor > dividend.
  k <- c < d
  jnzrel(k, done)

  // Prepare the value to subtract from the dividend.
  g <- d
build_subtrahend:
  // If the value is greater than the dividend, we're done.
  k <- c < g
  jnzrel(k, compute_quotient)

  // Otherwise, multiply by 2.
  g <- g << 1
  goto(build_subtrahend)

compute_quotient:
  // If our subtrahend is equal to our divisor, we're done.
  j <- g <  d
  k <- g == d
  k <- j | k
  jnzrel(k, done)

  // Divide the subtrahend by 2.
  g <- g >> 1

  // Check to see if we can subtract the subtrahend.
  k <- c < g
  jnzrel(k, shift_quotient)

  // Perform a subtraction and set this output bit to 1.
  c <- c - g
  b <- b | 1

  // Each iteration, shift the quotient left by one.
shift_quotient:
  b <- b << 1
  goto(compute_quotient)

  // Return result.
done:
  b <- b >> 1
  popall(d,g,j,k)
  ret

