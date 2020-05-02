#include "common.th"

    b <- 10
top:
    c <- b == 0
    b <- b - 1
    p <- @+top &~ c + p
done:
    illegal
