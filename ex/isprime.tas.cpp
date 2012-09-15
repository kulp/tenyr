#include "common.th"

.global isprime

start:
    prologue

    c <- 12
    call(isprime)

    illegal

// Checks whether C is prime or not. Returns a truth value in B.
isprime:
    // 0 and 1 are not prime.
    j <- c == 0
    k <- c == 1
    k <- j | k
    jnzrel(k, cleanup0)

    // 2 and 3 are prime.
    j <- c == 2
    k <- c == 3
    k <- j | k
    jnzrel(k, prime)

    // Check for divisibility by 2.
    push(c)
    d <- 2
    call(umod)
    k <- b == 0
    pop(c)
    jnzrel(k, cleanup0)

    // Check for divisibility by 3.
    push(c)
    d <- 3
    call(umod)
    k <- b == 0
    pop(c)
    jnzrel(k, cleanup0)

    // Compute upper bound and store it in M.
    push(c)
    call(isqrt)
    pop(c)
    m <- b

    i <- 1
mod_loop:
    // Check to see if we're done, i.e. 6*i - 1 > m
    g <- i * 6
    g <- g - 1
    k <- g > m
    jnzrel(k, prime)

    // Store the upper bound until the next iteration.
    push(m)

    // Check for divisibility by 6*i - 1.
    push(c)
    push(i)
    g <- d
    call(umod)
    k <- b == 0
    pop(i)
    pop(c)
    jnzrel(k, cleanup1)

    // Check for divisibility by 6*i + 1.
    push(c)
    push(i)
    d <- i * 6
    d <- d + 1
    call(umod)
    k <- b == 0
    pop(i)
    pop(c)

    // Increment i and continue.
    pop(m)
    jnzrel(k, cleanup0)
    i <- i + 1
    goto(mod_loop)

prime:
    b <- -1
    ret

cleanup1:
    pop(c)
cleanup0:
composite:
    b <- 0
    ret
