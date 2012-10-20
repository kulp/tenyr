#include "common.th"

    .global main
main:
    prologue

#define DATA_LEN (.L_data_end - .L_data_start)
#define ELT_LEN  (.L_data_elt_end - .L_data_elt_start)

    c <- rel(data_start)        // data to sort
    d <- (DATA_LEN / ELT_LEN)   // number of elements
    e <- ELT_LEN                // size of each element
    f <- rel(inteq)             // comparator
    call(qsort)

done:
    i <- .L_data_start
print_loop:
    c <- [i - (. + 0) + p]      // get second field of struct
    c <-  c - (. + 1) + p       // and relocate it
    call(puts)
    c <- rel(nl)
    call(puts)
    i <- i + ELT_LEN
    c <- i < .L_data_end
    jnzrel(c,print_loop)

    illegal

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

// c <- pointer to key
// d <- pointer to element
// b -> < 0, 0, > 0
inteq:
    c <- [c]
    d <- [d]
    b <- c - d
    ret

data_start:
.L_data_start:
.L_data_elt_start:
    .word     21, @L_21
.L_data_elt_end:
    .word     34, @L_34
    .word     55, @L_55
    .word    144, @L_144
    .word      1, @L_1
    .word      2, @L_2
    .word     89, @L_89
    .word      3, @L_3
    .word      5, @L_5
    .word      8, @L_8
    .word     13, @L_13
.L_data_end:
    .word 0

L_1  : .utf32 "one"                    ; .word 0
L_2  : .utf32 "two"                    ; .word 0
L_3  : .utf32 "three"                  ; .word 0
L_5  : .utf32 "five"                   ; .word 0
L_8  : .utf32 "eight"                  ; .word 0
L_13 : .utf32 "thirteen"               ; .word 0
L_21 : .utf32 "twenty-one"             ; .word 0
L_34 : .utf32 "thirty-four"            ; .word 0
L_55 : .utf32 "fifty-five"             ; .word 0
L_89 : .utf32 "eighty-nine"            ; .word 0
L_144: .utf32 "one hundred forty-four" ; .word 0

nl: .word '\n', 0

