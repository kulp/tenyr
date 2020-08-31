#include "common.th"

    c   <- 10
    push(p + 2); p <- @+buddy_malloc + p
    c   <- b
    push(p + 2); p <- @+buddy_free + p
    illegal

