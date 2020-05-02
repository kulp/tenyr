#include "common.th"

#define elem(Dest, Base, Index) \
    Dest <- E * Index         ; \
    Dest <- Dest + Base       ; \
    //

#define swap(i0, i1)         \
    pushall(c,d,e,f)       ; \
    f <- i0                ; \
    g <- i1                ; \
    call(do_swap)          ; \
    popall(c,d,e,f)          \
    //

do_swap:
    l <- c
    k <- e
    o <- o - e
    c <- o + 1
    elem(d,l,f)
    /* E is already width */
    call(memcpy)
    elem(c,l,f)
    elem(d,l,g)
    e <- k
    call(memcpy)
    elem(c,l,g)
    d <- o + 1
    e <- k
    call(memcpy)
    o <- o + k
    o <- o + 1
    p <- [o]

// c <- base
// d <- number of elements
// e <- size of element
// f <- comparator
#define SI M
#define II J
#define PI H
#define LI I
#define BASE C

    .global qsort
qsort:
    pushall(g,h,i,j,k,l,m)
    h <- d < 2                  // test for base case
    p <- @+L_qsort_done & h + p
    PI <- d >> 1                // partition index
    LI <- d - 1                 // last index
    // partitioning
    swap(h,i)
    SI <- 0                     // store index
    II <- 0                     // i index

L_qsort_partition:
    k <- II < LI
    p <- @+L_qsort_partition_done &~ k + p
    pushall(c)
    elem(k, BASE, II)
    elem(d, BASE, LI)
    c <- k
    callr(f)                    // call comparator
    popall(c)
    k <- b < 0
    p <- @+L_qsort_noswap &~ k + p
    swap(II,SI)
    SI <- SI + 1
L_qsort_noswap:
    II <- II + 1
    p <- p + @+L_qsort_partition

L_qsort_partition_done:
    swap(SI,LI)
    PI <- SI

    // recursive cases --------------------
    // save argument registers
    m <- c
    j <- e
    k <- f
    // C is already elem(c,c,0)
    d <- PI
    // E is already width
    // F is already callback
    call(qsort)
    // restore argument registers
    c <- m
    d <- LI - PI
    e <- j
    f <- k
    PI <- PI + 1
    elem(k,BASE,PI)
    c <- k
    // TODO tail call
    call(qsort)

L_qsort_done:
    popall_ret(g,h,i,j,k,l,m)

