#include "common.th"
#include "serial.th"

// argument in C

    .global puts
puts:
    push(d)
puts_loop:
    b <- [c]            // load word from string
    d <- b == 0         // if it is zero, we are done
    jnzrel(d,puts_done)
    c <- c + 1          // increment index for next time
    emit(b)             // output character to serial device
    goto(puts_loop)
puts_done:
    pop(d)
    ret

