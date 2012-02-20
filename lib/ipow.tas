#include "common.th"

.global pow

// Performs C^D and stores the result in B.
pow:
  // Initialize the return value to 1.
  b <- 1
  
  // Multiply the product by C until D == 0.
mult_loop:
  k <- d == 0
  jnzrel(k, done)
  
  // Check to see if the exponent is even or not.
  k <- d & 1
  k <- k - 1
  jnzrel(k, even)

  // If odd, subtract one from the exponent and multiply by the base.
odd:
  b <- b * c
  d <- d - 1
  goto(mult_loop)
  
  // If even, halve the exponent and square the base.
even:
  c <- c * c
  d <- d >> 1
  goto(mult_loop)

done:
  ret

