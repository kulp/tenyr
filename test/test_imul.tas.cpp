#include "common.th"

_start:
    prologue

    c <- 0x7ed
    d <- 0x234
    call(imul)

    illegal
