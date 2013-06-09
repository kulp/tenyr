#include "common.th"
#include "irq.th"

    B   <- -1
    B   -> [IMR_ADDR]

#include "fib.tas.cpp"

irq_00:
    A   <- 0xeee
    C   <- 1
    C   -> [ISR_ADDR]
    ret

