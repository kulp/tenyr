#include "common.th"

    .global convert_hex
convert_hex:
    push(d)
    b <- rel(tmpbuf_end)
convert_hex_top:
    b <- b - 1
    d <- c & 0xf
    d <- [d + rel(hexes)]
    d -> [b]
    c <- c >> 4
    d <- c == 0
    jzrel(d,convert_hex_top)

    pop(d)
    ret

hexes:
    .word '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

