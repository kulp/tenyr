#include "common.th"
#include "vga.th"

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

