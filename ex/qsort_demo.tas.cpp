#include "common.th"

    .global main
main:
    prologue

#define DATA_LEN (.L_data_end - .L_data_start)
#define ELT_LEN  (.L_data_elt_end - .L_data_elt_start)

    c <- @+data_start + p       // data to sort
    d <- (DATA_LEN / ELT_LEN)   // number of elements
    e <- ELT_LEN                // size of each element
    f <- @+inteq + p            // comparator
    call(qsort)

done:
    i <- .L_data_start
print_loop:
    c <- [i - (. + 0) + p]      // get second field of struct
    c <-  c - (. + 1) + p       // and relocate it
    call(puts)
    c <- @+nl + p
    call(puts)
    i <- i + ELT_LEN
    c <- i < .L_data_end
    p <- @+print_loop & c + p

    illegal

// c <- pointer to key
// d <- pointer to element
// b -> < 0, 0, > 0
inteq:
    c <- [c]
    d <- [d]
    b <- c - d
    o <- o + 1
    p <- [o]

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

L_1  : .chars "one"                    ; .word 0
L_2  : .chars "two"                    ; .word 0
L_3  : .chars "three"                  ; .word 0
L_5  : .chars "five"                   ; .word 0
L_8  : .chars "eight"                  ; .word 0
L_13 : .chars "thirteen"               ; .word 0
L_21 : .chars "twenty-one"             ; .word 0
L_34 : .chars "thirty-four"            ; .word 0
L_55 : .chars "fifty-five"             ; .word 0
L_89 : .chars "eighty-nine"            ; .word 0
L_144: .chars "one hundred forty-four" ; .word 0

nl: .word '\n', 0

