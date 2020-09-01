#include "common.th"
#include "vga.th"

_start:
    prologue
    b <- 0              // indicate non-completion to testbench

restart:
    c <- 0
    j <- @VGA_BASE
    k <- (ROWS * COLS)
    k <- k + j

top:
    c -> [j]
    c <- c + 1
    c <- c & 0xff

    j <- j + 1
    n <- j < k
    jnzrel(n,top)

    b <- -1             // indicate completion to testbench
    goto(restart)     // restartable, but exits to testbench by default
    illegal

