#include "common.th"
#include "serial.th"

// argument in C

    .global puts
puts:
    push(d)
puts_loop:
    b <- [c]            // load word from string
    d <- b == 0         // if it is zero, we are done
    p <- @+puts_done & d + p
    c <- c + 1          // increment index for next time
    emit(b)             // output character to serial device
    p <- p + @+puts_loop
puts_done:
    popall_ret(d)

