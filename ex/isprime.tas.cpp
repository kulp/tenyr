#include "common.th"

.global isprime

start:
    prologue

    c <- [@+large + p]
    call(isprime)

    illegal

large: .word 8675309

// Checks whether C is prime or not. Returns a truth value in B.
isprime:
    pushall(d,e,g,i,j,k,m)
    // use e as local copy of c so we don't have to constantly save and restore
    // it when calling other functions
    e <- c
    // 0 and 1 are not prime.
    j <- e == 0
    k <- e == 1
    k <- j | k
    jnzrel(k, cleanup0)

    // 2 and 3 are prime.
    j <- e == 2
    k <- e == 3
    k <- j | k
    jnzrel(k, prime)

    // Check for divisibility by 2.
    c <- e
    d <- 2
    call(umod)
    k <- b == 0
    jnzrel(k, cleanup0)

    // Check for divisibility by 3.
    c <- e
    d <- 3
    call(umod)
    k <- b == 0
    jnzrel(k, cleanup0)

    // Compute upper bound and store it in M.
    c <- e
    call(isqrt)
    m <- b

    i <- 1
mod_loop:
    // Check to see if we're done, i.e. 6*i - 1 > m
    g <- i * 6
    g <- g - 1
    k <- m < g
    jnzrel(k, prime)

    // Check for divisibility by 6*i - 1.
    c <- e
    g <- d
    call(umod)
    k <- b == 0
    jnzrel(k, cleanup1)

    // Check for divisibility by 6*i + 1.
    c <- e
    d <- i * 6
    d <- d + 1
    call(umod)
    k <- b == 0

    // Increment i and continue.
    jnzrel(k, cleanup0)
    i <- i + 1
    p <- p + @+mod_loop

prime:
    b <- -1
    p <- p + @+done

cleanup1:
    pop(c)
cleanup0:
composite:
    b <- 0
done:
    popall(d,e,g,i,j,k,m)
    ret
