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
    Dest <- Index * E         ; \
    Dest <- Dest + Base       ; \
    //

#define swap(i0, i1)            \
    pushall(c,d,e,f,g)        ; \
    f <- c                    ; \
    g <- e                    ; \
    o <- o - e - 1            ; \
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
    o <- o + g + 1            ; \
    popall(c,d,e,f,g)         ; \
    //

// c <- base
// d <- number of elements
// e <- size of element
// f <- comparator
    .global qsort
qsort:
    pushall(h,i,j,k,l,m)
    h <- d < 2                  // test for base case
    jnzrel(h,L_qsort_done)
    h <- d >> 1                 // H is partition index
    i <- d - 1                  // I is last index
    // partitioning
    swap(h,i)
    m <- 0                      // M is store index
    j <- 0                      // J is i index

L_qsort_partition:
    elem(k, c, j)
    elem(l, c, j)
    pushall(c,d,e)
    c <- k
    d <- l
    callr(f)
    popall(c,d,e)
    k <- b < 0
    jzrel(k,L_qsort_noswap)
    swap(j,m)
    m <- m + 1
L_qsort_noswap:

    j <- j + 1
    k <- j < i
    jnzrel(k,L_qsort_partition)

    swap(j,i)
L_qsort_done:
    popall(h,i,j,k,l,m)
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

