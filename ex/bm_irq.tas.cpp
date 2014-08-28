#include "common.th"
#include "irq.th"

    B   <- -1
    B   -> [IMR_ADDR]

#include "bm_fib.tas.cpp"

irq_00:
    A   <- 0x0ee
    C   <- (1 << 0)
    C   -> [ISR_ADDR]
    ret

counter_02: .word 0
irq_02:
    C   <- (1 << 2)
    C   -> [ISR_ADDR]
    pushall(d,e,f)
    d <- 30
    e <- 30
    f <- [rel(counter_02)]
    push(f)
    call(putnum)
    pop(f)
    f <- f + 1
    f -> [rel(counter_02)]
    popall(d,e,f)

    ret

irq_03:
    C   <- (1 << 3)
    C   -> [ISR_ADDR]
    pushall(d,e,f)
    d <- 30
    e <- 30
    f <- [rel(counter_02)]
    push(f)
    call(putnum)
    pop(f)
    f <- f + 16
    f -> [rel(counter_02)]
    popall(d,e,f)

    ret

