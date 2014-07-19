#include "common.th"

// -----------------------------------------------------------------------------
.global init_display
init_display:
    pushall(h,k,l,m);
    h <- 0x100
    h <- h << 8         // h is video base
    k <- 0              // k is offset into text region

init_display_loop:
    m <- h + k + 0x10   // m is k characters past start of text region
    [m] <- ' '          // write space to display
    k <- k + 1          // go to the right one character
    l <- 0x20 < k       // shall we loop ?
    jzrel(l,init_display_loop)

init_display_done:
    popall(h,k,l,m);
    ret

.global disable_cursor
disable_cursor:
    pushall(g,h)
    h <- 0x100
    g <- [h << 8]
    g <- g &~ (1 << 6)  // unset cursor-enable bit
    g -> [h << 8]
    popall(g,h)
    ret

