#include "common.th"

// c <- key
// d <- base
// e <- number of elements
// f <- size of element
// g <- comparator
// b -> pointer or null
.global bsearch
bsearch:
    o <- o - 3
    h -> [o + (3 - 0)]
    i -> [o + (3 - 1)]
    j -> [o + (3 - 2)]  // callee-save temps

bsearch_loop:
    // c is the key ptr
    // d is the first element to consider
    // e is the number of elements to consider after d (31-bit unsigned)
    i <- e == 0
    p <- @+bsearch_notfound & i + p

    o <- o - 4
    c -> [o + (4 - 0)]
    d -> [o + (4 - 1)]
    e -> [o + (4 - 2)]
    f -> [o + (4 - 3)]
    // consider element halfway between (d) and (d + e)
    i <- e >> 1
    i <- i * f
    d <- d + i
    push(d)     // save testpointer
    push(p + 2)
    p <- g      // call indirect
    i <- b      // copy result to temp
    pop(b)      // restore testpointer to b in case of match
    o <- o + 4
    f <- [o - 3]
    e <- [o - 2]
    d <- [o - 1]
    c <- [o - 0]

    j <- i == 0
    p <- @+bsearch_done & j + p
    j <- i < 0
    p <- @+bsearch_less & j + p

    e <- e + 1
    e <- e >> 1
    j <- e * f
    d <- d + j
    p <- p + @+bsearch_loop

bsearch_less:
    e <- e >> 1
    p <- p + @+bsearch_loop

bsearch_notfound:
    b <- 0
    p <- p + @+bsearch_done

bsearch_done:
    o <- o + 4
    j <- [o - (1 + 2)]
    i <- [o - (1 + 1)]
    h <- [o - (1 + 0)]
    p <- [o]

