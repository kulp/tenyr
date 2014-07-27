#include "common.th"
#define COLS 80
#define ROWS 40

// -----------------------------------------------------------------------------
.global init_display
init_display:
    pushall(h,k,n)
    h <- 0x100
    h <- h << 8         // h is video base + row
    n <- ((ROWS * COLS) / 2)
    n <- n << 1 + h     // add boundary in two pieces

init_display_loop:
    [h] <- ' '          // write space to display
    h <- h + 1          // go to the right one character
    k <- h < n          // shall we loop ?
    jnzrel(k,init_display_loop)

    popall_ret(h,k,n)

.global disable_cursor
disable_cursor:
    pushall(g,h)
    h <- 0x110
    h <- h << 8
    g <- [h - 1]
    g <- g & ~(1 << 6)  // unset cursor-enable bit
    g -> [h - 1]
    popall_ret(g,h)

