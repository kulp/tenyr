#include "common.th"
#include "serial.th"

#define CHANNEL "#tenyr"
#define NICK    "rynet"

#define do(X) c <- rel(X) ; call(puts) ; c <- rel(rn) ; call(puts)

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
    d <- rel(ping)
    e <- 5
    call(strncmp)
    jzrel(b,do_pong)

    c <- g
    d <- 1
    call(skipwords)

    c <- b
    d <- rel(privmsg)
    e <- 7
    call(strncmp)
    jnzrel(b,loop_line)
    c <- g
    d <- 3
    call(skipwords)

    c <- b + 1
    call(check_input)

    goto(loop_line)

    illegal
do_pong:
    c <- rel(pong)
    call(puts)
    c <- g
    d <- 1
    call(skipwords)
    c <- b
    call(puts) // respond with same identifier
    c <- rel(rn)
    call(puts)
    goto(loop_line)

check_input:
    pushall(d,e)
    push(c)
    d <- rel(trigger)
    e <- 3 // strlen(trigger)
    call(strncmp)
    pop(c)
    jzrel(b,check_input_triggered)
    goto(check_input_done)

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
    popall(d,e)
    ret

ping: .chars "PING " ; .word 0
pong: .chars "PONG " ; .word 0

tmpbuf: .chars "0123456789abcdef"
tmpbuf_end: .word 0

say_decimal:
    call(say_start)
    call(convert_decimal)
    c <- b
    call(puts)
    call(say_end)
    ret

say_fahrenheit:
    call(say_start)
    call(convert_decimal)
    c <- b
    call(puts)
    c <- rel(degF)
    call(puts)
    call(say_end)
    ret

// TODO this is actually being encoded as UTF-8
degF: .chars "°F" ; .word 0

convert_decimal:
    pushall(d,f,g,h)
    g <- c
    h <- rel(tmpbuf_end)
    d <- c < 0
    jnzrel(d,convert_decimal_negative)
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
    jzrel(d,convert_decimal_top)
    jzrel(f,convert_decimal_done)
    h <- h - 1
    [h] <- '-'

convert_decimal_done:
    b <- h
    popall(d,f,g,h)
    ret
convert_decimal_negative:
    g <- - g
    f <- -1
    goto(convert_decimal_top)

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
    pushall(e,g)
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
    popall(e,g)
    ret

getline:
    pushall(c,d,f)
    f <- rel(buffer)

getline_top:
    getch(b)
    c <- b == '\r'
    c <- b == '\n' + c
    jnzrel(c,getline_eol)
    b -> [f]
    f <- f + 1
    goto(getline_top)

getline_eol:
    a -> [f]
    b <- rel(buffer)
    popall(c,d,f)
    ret

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

