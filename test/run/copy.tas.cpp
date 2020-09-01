_start:
    o <- ((1 << 13) - 1)
    c <- @+dst + p
    d <- @+src + p
    e <- 10
    call(memcpy)
    c <- b
    call(puts)
    illegal

dst: .chars "                " ; .word 0
src: .chars "0123456789ABCDEF" ; .word 0

