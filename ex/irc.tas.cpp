#include "common.th"
#include "serial.th"

#define CHANNEL "#tenyr"
#define NICK    "rynet"

#define do(X) c <- @+X + p  ; push(p + 2) ; p <- @+puts + p ; \
              c <- @+rn + p ; push(p + 2) ; p <- @+puts + p

_start:
    prologue

    do(nick)
    do(user)
    do(join)

loop_line:
    push(p + 2); p <- @+getline + p
    // use g as backing store for line ; c is stompable as arg
    g <- b

    c <- g
    d <- @+ping + p
    e <- 5
    push(p + 2); p <- @+strncmp + p
    p <- @+do_pong &~ b + p

    c <- g
    d <- 1
    push(p + 2); p <- @+skipwords + p

    c <- b
    d <- @+privmsg + p
    e <- 7
    push(p + 2); p <- @+strncmp + p
    p <- @+loop_line & b + p
    c <- g
    d <- 3
    push(p + 2); p <- @+skipwords + p

    c <- b + 1
    push(p + 2); p <- @+check_input + p

    p <- p + @+loop_line

    illegal
do_pong:
    c <- @+pong + p
    push(p + 2); p <- @+puts + p
    c <- g
    d <- 1
    push(p + 2); p <- @+skipwords + p
    c <- b
    push(p + 2); p <- @+puts + p // respond with same identifier
    c <- @+rn + p
    push(p + 2); p <- @+puts + p
    p <- p + @+loop_line

check_input:
    o <- o - 2
    d -> [o + (2 - 0)]
    e -> [o + (2 - 1)]
    push(c)
    d <- @+trigger + p
    e <- 3 // strlen(trigger)
    push(p + 2); p <- @+strncmp + p
    pop(c)
    p <- @+check_input_triggered &~ b + p
    p <- p + @+check_input_done

check_input_triggered:
    c <- c + 3
    d <- 0
    e <- 10
    push(p + 2); p <- @+strtol + p
    c <- b + 40
    c <- c * 9
    d <- 5
    push(p + 2); p <- @+udiv + p
    c <- b - 40
    push(p + 2); p <- @+say_fahrenheit + p

check_input_done:
    o <- o + 3
    e <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]

ping: .chars "PING " ; .word 0
pong: .chars "PONG " ; .word 0

tmpbuf: .chars "0123456789abcdef"
tmpbuf_end: .word 0

say_decimal:
    push(p + 2); p <- @+say_start + p
    push(p + 2); p <- @+convert_decimal + p
    c <- b
    push(p + 2); p <- @+puts + p
    push(p + 2); p <- @+say_end + p
    o <- o + 1
    p <- [o]

say_fahrenheit:
    push(p + 2); p <- @+say_start + p
    push(p + 2); p <- @+convert_decimal + p
    c <- b
    push(p + 2); p <- @+puts + p
    c <- @+degF + p
    push(p + 2); p <- @+puts + p
    push(p + 2); p <- @+say_end + p
    o <- o + 1
    p <- [o]

// TODO this is actually being encoded as UTF-8
degF: .chars "°F" ; .word 0

convert_decimal:
    o <- o - 4
    d -> [o + (4 - 0)]
    f -> [o + (4 - 1)]
    g -> [o + (4 - 2)]
    h -> [o + (4 - 3)]
    g <- c
    h <- @+tmpbuf_end + p
    d <- c < 0
    p <- @+convert_decimal_negative & d + p
    f <- 0
convert_decimal_top:
    h <- h - 1

    c <- g
    d <- 10
    push(p + 2); p <- @+umod + p

    [h] <- b + '0'

    c <- g
    d <- 10
    push(p + 2); p <- @+udiv + p
    g <- b

    d <- g == 0
    p <- @+convert_decimal_top &~ d + p
    p <- @+convert_decimal_done &~ f + p
    h <- h - 1
    [h] <- '-'

convert_decimal_done:
    b <- h
    o <- o + 5
    h <- [o - (1 + 3)]
    g <- [o - (1 + 2)]
    f <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]
convert_decimal_negative:
    g <- - g
    f <- -1
    p <- p + @+convert_decimal_top

say:
    push(p + 2); p <- @+say_start + p
    push(p + 2); p <- @+puts + p
    push(p + 2); p <- @+say_end + p
    o <- o + 1
    p <- [o]

say_start:
    push(c)
    c <- @+talk + p
    push(p + 2); p <- @+puts + p
    o <- o + 2
    c <- [o - (1 + 0)]
    p <- [o]

say_end:
    push(c)
    c <- @+rn + p
    push(p + 2); p <- @+puts + p
    o <- o + 2
    c <- [o - (1 + 0)]
    p <- [o]

skipwords:
    o <- o - 2
    e -> [o + (2 - 0)]
    g -> [o + (2 - 1)]
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
    o <- o + 3
    g <- [o - (1 + 1)]
    e <- [o - (1 + 0)]
    p <- [o]

getline:
    o <- o - 3
    c -> [o + (3 - 0)]
    d -> [o + (3 - 1)]
    f -> [o + (3 - 2)]
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
    o <- o + 4
    f <- [o - (1 + 2)]
    d <- [o - (1 + 1)]
    c <- [o - (1 + 0)]
    p <- [o]

// ----------------------------------------------------------------------------

buffer: // 512 chars
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"
    .chars "----------------------------------------------------------------"

nick: .chars "NICK " NICK ; .word 0
user: .chars "USER " NICK " 0 * :get your own bot" ; .word 0
join: .chars "JOIN " CHANNEL ; .word 0
talk: .chars "PRIVMSG " CHANNEL " :" ; .word 0

privmsg: .chars "PRIVMSG" ; .word 0
trigger: .chars "!f " ; .word 0

rn: .word '\r', '\n', 0

