.global isprime

start:
    o <- ((1 << 13) - 1)

    c <- [@+large + p]
    [o] <- p + 2 ; o <- o - 1 ; p <- @+isprime + p

    illegal

large: .word 8675309

// Checks whether C is prime or not. Returns a truth value in B.
isprime:
    o <- o - 7
    d -> [o + (7 - 0)]
    e -> [o + (7 - 1)]
    g -> [o + (7 - 2)]
    i -> [o + (7 - 3)]
    j -> [o + (7 - 4)]
    k -> [o + (7 - 5)]
    m -> [o + (7 - 6)]
    // use e as local copy of c so we don't have to constantly save and restore
    // it when calling other functions
    e <- c
    // 0 and 1 are not prime.
    j <- e == 0
    k <- e == 1
    k <- j | k
    p <- @+cleanup0 & k + p

    // 2 and 3 are prime.
    j <- e == 2
    k <- e == 3
    k <- j | k
    p <- @+prime & k + p

    // Check for divisibility by 2.
    c <- e
    d <- 2
    [o] <- p + 2 ; o <- o - 1 ; p <- @+umod + p
    k <- b == 0
    p <- @+cleanup0 & k + p

    // Check for divisibility by 3.
    c <- e
    d <- 3
    [o] <- p + 2 ; o <- o - 1 ; p <- @+umod + p
    k <- b == 0
    p <- @+cleanup0 & k + p

    // Compute upper bound and store it in M.
    c <- e
    [o] <- p + 2 ; o <- o - 1 ; p <- @+isqrt + p
    m <- b

    i <- 1
mod_loop:
    // Check to see if we're done, i.e. 6*i - 1 > m
    g <- i * 6
    g <- g - 1
    k <- m < g
    p <- @+prime & k + p

    // Check for divisibility by 6*i - 1.
    c <- e
    g <- d
    [o] <- p + 2 ; o <- o - 1 ; p <- @+umod + p
    k <- b == 0
    p <- @+cleanup1 & k + p

    // Check for divisibility by 6*i + 1.
    c <- e
    d <- i * 6
    d <- d + 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+umod + p
    k <- b == 0

    // Increment i and continue.
    p <- @+cleanup0 & k + p
    i <- i + 1
    p <- p + @+mod_loop

prime:
    b <- -1
    p <- p + @+done

cleanup1:
    o <- o + 1 ; c <- [o]
cleanup0:
composite:
    b <- 0
done:
    o <- o + 8
    m <- [o - (1 + 6)]
    k <- [o - (1 + 5)]
    j <- [o - (1 + 4)]
    i <- [o - (1 + 3)]
    g <- [o - (1 + 2)]
    e <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]
