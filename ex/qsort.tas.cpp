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

// c <- base
// d <- number of elements
// e <- size of element
// f <- comparator
    .global qsort
qsort:
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

