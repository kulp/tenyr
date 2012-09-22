#include "common.th"
#include "serial.th"

#define CHANNEL "#tenyr"
#define NICK    "rynet"

#define do(X) c <- rel(X) ; call(puts) ; c <- rel(rn) ; call(puts)

_start:
    prologue

#if 0
    c <- 2
    call(sleep)
#endif

    do(nick)
    do(user)
    do(join)

loop_line:
    call(getline)
    c <- b
    push(c)
    d <- 1
    call(skipwords)

    c <- b
    d <- rel(privmsg)
    e <- 7
    call(strncmp)
    pop(c)
    jnzrel(b,loop_line)
    d <- 3
    call(skipwords)

    c <- b + 1
    call(check_input)

    goto(loop_line)

    illegal

check_input:
    push(c)
    d <- rel(trigger)
    e <- 5 // strlen(trigger)
    call(strncmp)
    pop(c)
    jzrel(b,triggered)
    ret

triggered:
    push(c)
    c <- c + 5
    call(parse_int)
    c <- b
    call(say_hex)
    pop(c)
    ret

    ret

// c <- address of first character of number
parse_int:
    b <- 0
parse_int_top:
    d <- [c]
    c <- c + 1
    d <- d - '0'
    e <- d > 9
    jnzrel(e,parse_int_done)
    e <- d < 0
    jnzrel(e,parse_int_done)
    b <- b * 10 + d
    goto(parse_int_top)
parse_int_done:
    ret

say_hex:
    call(say_start)
    push(c)
    c <- rel(zero_x)
    call(puts)
    pop(c)
    call(convert_hex)
    c <- b
    call(puts)
    call(say_end)
    ret

zero_x: .utf32 "0x" ; .word 0

convert_hex:
    b <- rel(tmpbuf_end)
convert_hex_top:
    b <- b - 1
    d <- c & 0xf
    d <- [d + rel(hexes)]
    d -> [b]
    c <- c >> 4
    d <- c == 0
    jzrel(d,convert_hex_top)

    ret

hexes:
    .word '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

say:
    call(say_start)
    call(puts)
    call(say_end)
    ret

say_start:
    push(c)
    c <- rel(talk)
    call(puts)
    pop(c)
    ret

say_end:
    push(c)
    c <- rel(rn)
    call(puts)
    pop(c)
    ret

skipwords:
skipwords_top:
    g <- [c]
    e <- g == 0
    jnzrel(e,skipwords_done)
    e <- g == ' '
    c <- c + 1
    jnzrel(e,skipwords_foundspace)
    goto(skipwords_top)

skipwords_foundspace:
    d <- d - 1
    e <- d < 1
    jzrel(e,skipwords_top)

skipwords_done:
    b <- c
    ret

//  :usermask PRIVMSG #channel :message
getline:
    g <- rel(buffer)
    f <- g

getline_top:
    getch(b)
    c <- b == '\r'
    d <- b == '\n'
    c <- c | d
    jnzrel(c,getline_eol)
    b -> [f]
    f <- f + 1
    goto(getline_top)

getline_eol:
    a -> [f]
    b <- g
    ret

sleep:
    c <- c * 2047
    c <- c * 2047
sleep_loop:
    c <- c - 1
    d <- c == 0
    jnzrel(d,sleep_done)
    goto(sleep_loop)
sleep_done:
    ret

// ----------------------------------------------------------------------------

buffer: // 512 chars
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"
    .utf32 "----------------------------------------------------------------"

nick: .utf32 "NICK " NICK ; .word 0
user: .utf32 "USER " NICK " 0 * :get your own bot" ; .word 0
join: .utf32 "JOIN " CHANNEL ; .word 0
talk: .utf32 "PRIVMSG " CHANNEL " :" ; .word 0

privmsg: .utf32 "PRIVMSG" ; .word 0
trigger: .utf32 "!hex " ; .word 0

rn: .word '\r', '\n', 0

