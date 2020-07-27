#include "common.th"
#include "vga.th"

// -----------------------------------------------------------------------------
.global init_display
init_display:
    o <- o - 3
    h -> [o + (3 - 0)]
    k -> [o + (3 - 1)]
    n -> [o + (3 - 2)]
    h <- 0x100
    h <- h << 8         // h is video base + row
    n <- ((ROWS * COLS) / 2)
    n <- n << 1 + h     // add boundary in two pieces

init_display_loop:
    [h] <- ' '          // write space to display
    h <- h + 1          // go to the right one character
    k <- h < n          // shall we loop ?
    p <- @+init_display_loop & k + p

    o <- o + 4
    n <- [o - (1 + 2)]
    k <- [o - (1 + 1)]
    h <- [o - (1 + 0)]
    p <- [o]

