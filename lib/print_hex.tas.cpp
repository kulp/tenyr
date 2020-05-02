#include "common.th"

    .global convert_hex
convert_hex:
    push(d)
    b <- @+tmpbuf_end + p
convert_hex_top:
    b <- b - 1
    d <- c & 0xf
    d <- [d + @+hexes + p]
    d -> [b]
    c <- c >>> 4
    d <- c == 0
    p <- @+convert_hex_top &~ d + p

    pop(d)
    ret

hexes:
    .word '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

