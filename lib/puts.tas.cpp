.set SERIAL, 1 << 5

// argument in C

    .global puts
puts:
    [o] <- d ; o <- o - 1
puts_loop:
    b <- [c]            // load word from string
    d <- b == 0         // if it is zero, we are done
    p <- @+puts_done & d + p
    c <- c + 1          // increment index for next time
    b -> [@SERIAL]      // output character to serial device
    p <- p + @+puts_loop
puts_done:
    o <- o + 2
    d <- [o - (1 + 0)]
    p <- [o]

