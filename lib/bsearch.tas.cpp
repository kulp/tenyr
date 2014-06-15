#include "common.th"

// c <- key
// d <- base
// e <- number of elements
// f <- size of element
// g <- comparator
// b -> pointer or null
bsearch:
    pushall(h,i,j)  // callee-save temps

bsearch_loop:
    // c is the key ptr
    // d is the first element to consider
    // e is the number of elements to consider after d (31-bit unsigned)
    i <- e == 0
    jnzrel(i,bsearch_notfound)

    pushall(c,d,e,f)
    // consider element halfway between (d) and (d + e)
    i <- e >> 1
    i <- i * f
    d <- d + i
    push(d)     // save testpointer
    callr(g)
    i <- b      // copy result to temp
    pop(b)      // restore testpointer to b in case of match
    popall(c,d,e,f)

    j <- i == 0
    jnzrel(j,bsearch_done)
    j <- i < 0
    jnzrel(j,bsearch_less)

    e <- e + 1
    e <- e >> 1
    j <- e * f
    d <- d + j
    goto(bsearch_loop)

bsearch_less:
    e <- e >> 1
    goto(bsearch_loop)

bsearch_notfound:
    b <- 0
    goto(bsearch_done)

bsearch_done:
    popall(h,i,j)
    ret

