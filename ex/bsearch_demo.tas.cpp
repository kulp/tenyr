#include "common.th"

    .global main
main:
    prologue

#define DATA_LEN (.L_data_end - .L_data_start)
#define ELT_LEN  (.L_data_elt_end - .L_data_elt_start)

    c <- 0                      // needle
    i <- [@+largest + p]
loop_top:
    h <- i < c
    p <- @+loop_exit & h + p

    c -> [@+key + p]            // update key value
    c <- @+key + p              // pointer to value
    d <- @+data_start + p       // haystack
    e <- (DATA_LEN / ELT_LEN)   // number of elements
    f <- ELT_LEN                // size of each element
    g <- @+inteq + p            // comparator
    [o] <- p + 2 ; o <- o - 1 ; p <- @+bsearch + p

    [o] <- c ; o <- o - 1
    c <- b == 0
    p <- @+notfound & c + p
    c <- [b + 1]
    c <- c - (. + 1) + p        // relocate string address argument
    p <- p + @+done

notfound:
    c <- @+error_msg + p

done:
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    c <- @+nl + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1 ; c <- [o]
    c <- [@+key + p]
    c <- c + 1                  // increment loop counter
    p <- p + @+loop_top
loop_exit:

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

error_msg:
    .chars "error : not found" ; .word 0

nl: .word '\n', 0

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
largest:
    .word 144, @L_144
.L_data_end:
    .word 0 // TODO why can't key overlap this word ? XXX bug
key:
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

