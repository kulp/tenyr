#include "common.th"

_start:
    prologue

#define DATA_LEN (.L_data_end - .L_data_start)
#define ELT_LEN  (.L_data_elt_end - .L_data_elt_start)

    c <- rel(key)               // needle
    d <- rel(data_start)        // haystack
    e <- (DATA_LEN / ELT_LEN)   // number of elements
    f <- ELT_LEN                // size of each element
    g <- rel(inteq)             // comparator
    call(bsearch)

    c <- b == 0
    jnzrel(c,notfound)
    c <- [b + 1]
    c <- c - (. + 1) + p        // relocate string address argument
    goto(done)

notfound:
    c <- rel(error_msg)

done:
    call(puts)
    c <- rel(nl)
    call(puts)

    illegal

error_msg:
    .ascii "error : not found" ; .word 0

nl: .word '\n', 0

key: .word 55

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
    // e is the number of elements to consider after d
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
    .word   1, @L_1
.L_data_elt_end:
    .word   2, @L_2
    .word   3, @L_3
    .word   5, @L_5
    .word   8, @L_8
    .word  13, @L_13
    .word  21, @L_21
    .word  34, @L_34
    .word  55, @L_55
    .word  89, @L_89
    .word 144, @L_144
.L_data_end:
    .word 0

L_1  : .ascii "one"                    ; .word 0
L_2  : .ascii "two"                    ; .word 0
L_3  : .ascii "three"                  ; .word 0
L_5  : .ascii "five"                   ; .word 0
L_8  : .ascii "eight"                  ; .word 0
L_13 : .ascii "thirteen"               ; .word 0
L_21 : .ascii "twenty-one"             ; .word 0
L_34 : .ascii "thirty-four"            ; .word 0
L_55 : .ascii "fifty-five"             ; .word 0
L_89 : .ascii "eighty-nine"            ; .word 0
L_144: .ascii "one hundred forty-four" ; .word 0

