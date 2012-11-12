#include "common.th"
#include "errno.th"

    c   <- 10
    call(buddy_malloc)
    c   <- b
    call(buddy_free)
    illegal

