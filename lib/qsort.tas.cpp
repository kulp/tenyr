#include "common.th"

#define elem(Dest, Base, Index) \
    Dest <- E * Index         ; \
    Dest <- Dest + Base       ; \
    //

// TODO convert swap to a function ?
// XXX swap() macros is not param-safe
#define swap(i0, i1)            \
    pushall(c,d,e,f,g)        ; \
    f <- c                    ; \
    g <- e                    ; \
    o <- o - e                ; \
    c <- o + 1                ; \
    elem(d,f,i0)              ; \
    /* E is already width */  ; \
    call(memcpy)              ; \
    elem(c,f,i0)              ; \
    elem(d,f,i1)              ; \
    e <- g                    ; \
    call(memcpy)              ; \
    elem(c,f,i1)              ; \
    d <- o + 1                ; \
    e <- g                    ; \
    call(memcpy)              ; \
    o <- o + g                ; \
    popall(c,d,e,f,g)         ; \
    //

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
    pushall(h,i,j,k,m)
    h <- d < 2                  // test for base case
    jnzrel(h,L_qsort_done)
    PI <- d >> 1                // partition index
    LI <- d - 1                 // last index
    // partitioning
    swap(h,i)
    SI <- 0                     // store index
    II <- 0                     // i index

L_qsort_partition:
    k <- II < LI
    jzrel(k,L_qsort_partition_done)
    pushall(c)
    elem(k, BASE, II)
    elem(d, BASE, LI)
    c <- k
    callr(f)                    // call comparator
    popall(c)
    k <- b < 0
    jzrel(k,L_qsort_noswap)
    swap(II,SI)
    SI <- SI + 1
L_qsort_noswap:
    II <- II + 1
    goto(L_qsort_partition)

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
    popall(h,i,j,k,m)
    ret

