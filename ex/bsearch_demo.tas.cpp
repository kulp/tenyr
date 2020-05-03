#include "common.th"

    .global main
main:
    prologue

.set DATA_LEN, (.L_data_end - .L_data_start)
.set ELT_LEN,  (.L_data_elt_end - .L_data_elt_start)

    c <- 0                      // needle
    i <- [rel(largest)]
loop_top:
    h <- i < c
    jnzrel(h, loop_exit)

    c -> [rel(key)]             // update key value
    c <- rel(key)               // pointer to value
    d <- rel(data_start)        // haystack
    e <- ($DATA_LEN / $ELT_LEN) // number of elements
    f <- $ELT_LEN               // size of each element
    g <- rel(inteq)             // comparator
    call(bsearch)

    push(c)
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
    pop(c)
    c <- [rel(key)]
    c <- c + 1                  // increment loop counter
    goto(loop_top)
loop_exit:

    illegal

// c <- pointer to key
// d <- pointer to element
// b -> < 0, 0, > 0
inteq:
    c <- [c]
    d <- [d]
    b <- c - d
    ret

error_msg:
    .utf32 "error : not found" ; .word 0

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

