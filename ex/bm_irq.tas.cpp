#include "common.th"
#include "irq.th"

    B   <- -1
    B   -> [IMR_ADDR]

#include "fib.tas.cpp"

irq_00:
    A   <- 0xeee
    C   <- (1 << 0)
    C   -> [ISR_ADDR]
    ret

irq_02:
    A   <- 0xfff
    C   <- (1 << 2)
    C   -> [ISR_ADDR]
    ret

