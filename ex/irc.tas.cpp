#include "common.th"
#include "serial.th"

#define CHANNEL "#tenyr"
#define NICK    "rynet"

#define do(X) c <- @+X + p ; call(puts) ; c <- @+rn + p ; call(puts)

_start:
    prologue

    do(nick)
    do(user)
    do(join)

loop_line:
    call(getline)
    // use g as backing store for line ; c is stompable as arg
    g <- b

    c <- g
    d <- @+ping + p
    e <- 5
    call(strncmp)
    p <- @+do_pong &~ b + p

    c <- g
    d <- 1
    call(skipwords)

    c <- b
    d <- @+privmsg + p
    e <- 7
    call(strncmp)
    p <- @+loop_line & b + p
    c <- g
    d <- 3
    call(skipwords)

    c <- b + 1
    call(check_input)

    p <- p + @+loop_line

    illegal
do_pong:
    c <- @+pong + p
    call(puts)
    c <- g
    d <- 1
    call(skipwords)
    c <- b
    call(puts) // respond with same identifier
    c <- @+rn + p
    call(puts)
    p <- p + @+loop_line

check_input:
    pushall(d,e)
    push(c)
    d <- @+trigger + p
    e <- 3 // strlen(trigger)
    call(strncmp)
    pop(c)
    p <- @+check_input_triggered &~ b + p
    p <- p + @+check_input_done

check_input_triggered:
    c <- c + 3
    d <- 0
    e <- 10
    call(strtol)
    c <- b + 40
    c <- c * 9
    d <- 5
    call(udiv)
    c <- b - 40
    call(say_fahrenheit)

check_input_done:
    popall_ret(d,e)

ping: .utf32 "PING " ; .word 0
pong: .utf32 "PONG " ; .word 0

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

say_decimal:
    call(say_start)
    call(convert_decimal)
    c <- b
    call(puts)
    call(say_end)
    o <- o + 1
    p <- [o]

say_fahrenheit:
    call(say_start)
    call(convert_decimal)
    c <- b
    call(puts)
    c <- @+degF + p
    call(puts)
    call(say_end)
    o <- o + 1
    p <- [o]

// TODO this is actually being encoded as UTF-8
degF: .utf32 "°F" ; .word 0

convert_decimal:
    pushall(d,f,g,h)
    g <- c
    h <- @+tmpbuf_end + p
    d <- c < 0
    p <- @+convert_decimal_negative & d + p
    f <- 0
convert_decimal_top:
    h <- h - 1

    c <- g
    d <- 10
    call(umod)

    [h] <- b + '0'

    c <- g
    d <- 10
    call(udiv)
    g <- b

    d <- g == 0
    p <- @+convert_decimal_top &~ d + p
    p <- @+convert_decimal_done &~ f + p
    h <- h - 1
    [h] <- '-'

convert_decimal_done:
    b <- h
    popall_ret(d,f,g,h)
convert_decimal_negative:
    g <- - g
    f <- -1
    p <- p + @+convert_decimal_top

say:
    call(say_start)
    call(puts)
    call(say_end)
    o <- o + 1
    p <- [o]

say_start:
    push(c)
    c <- @+talk + p
    call(puts)
    popall_ret(c)

say_end:
    push(c)
    c <- @+rn + p
    call(puts)
    popall_ret(c)

skipwords:
    pushall(e,g)
skipwords_top:
    g <- [c]
    e <- g == 0
    p <- @+skipwords_done & e + p
    e <- g == ' '
    c <- c + 1
    p <- @+skipwords_foundspace & e + p
    p <- p + @+skipwords_top

skipwords_foundspace:
    d <- d - 1
    e <- d < 1
    p <- @+skipwords_top &~ e + p

skipwords_done:
    b <- c
    popall_ret(e,g)

getline:
    pushall(c,d,f)
    f <- @+buffer + p

getline_top:
    getch(b)
    c <- b == '\r'
    c <- b == '\n' + c
    p <- @+getline_eol & c + p
    b -> [f]
    f <- f + 1
    p <- p + @+getline_top

getline_eol:
    a -> [f]
    b <- @+buffer + p
    popall_ret(c,d,f)

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
trigger: .utf32 "!f " ; .word 0

rn: .word '\r', '\n', 0

