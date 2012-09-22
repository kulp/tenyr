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
    push(c)
    c <- rel(talk)
    call(puts)
    pop(c)
    call(puts)
    c <- rel(rn)
    call(puts)
    goto(loop_line)

    illegal

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

rn: .word '\r', '\n', 0

