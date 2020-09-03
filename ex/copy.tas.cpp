_start:
    o <- ((1 << 13) - 1)
    c <- @+dst + p
    d <- @+src + p
    e <- 10
    [o] <- p + 2 ; o <- o - 1 ; p <- @+memcpy + p
    c <- b
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    illegal

dst: .chars "                " ; .word 0
src: .chars "0123456789ABCDEF" ; .word 0

