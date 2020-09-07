.set SERIAL, 1 << 5

#define CHANNEL "#tenyr"
#define NICK    "rynet"

#define do(X) c <- @+X + p  ; [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p ; \
              c <- @+rn + p ; [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p

_start:
    o <- ((1 << 13) - 1)

    do(nick)
    do(user)
    do(join)

loop_line:
    [o] <- p + 2 ; o <- o - 1 ; p <- @+getline + p
    // use g as backing store for line ; c is stompable as arg
    g <- b

    c <- g
    d <- @+ping + p
    e <- 5
    [o] <- p + 2 ; o <- o - 1 ; p <- @+strncmp + p
    p <- @+do_pong &~ b + p

    c <- g
    d <- 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+skipwords + p

    c <- b
    d <- @+privmsg + p
    e <- 7
    [o] <- p + 2 ; o <- o - 1 ; p <- @+strncmp + p
    p <- @+loop_line & b + p
    c <- g
    d <- 3
    [o] <- p + 2 ; o <- o - 1 ; p <- @+skipwords + p

    c <- b + 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+check_input + p

    p <- p + @+loop_line

    illegal
do_pong:
    c <- @+pong + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    c <- g
    d <- 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+skipwords + p
    c <- b
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p // respond with same identifier
    c <- @+rn + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    p <- p + @+loop_line

check_input:
    o <- o - 2
    d -> [o + (2 - 0)]
    e -> [o + (2 - 1)]
    [o] <- c ; o <- o - 1
    d <- @+trigger + p
    e <- 3 // strlen(trigger)
    [o] <- p + 2 ; o <- o - 1 ; p <- @+strncmp + p
    o <- o + 1 ; c <- [o]
    p <- @+check_input_triggered &~ b + p
    p <- p + @+check_input_done

check_input_triggered:
    c <- c + 3
    d <- 0
    e <- 10
    [o] <- p + 2 ; o <- o - 1 ; p <- @+strtol + p
    c <- b + 40
    c <- c * 9
    d <- 5
    [o] <- p + 2 ; o <- o - 1 ; p <- @+udiv + p
    c <- b - 40
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_fahrenheit + p

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
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_start + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+convert_decimal + p
    c <- b
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_end + p
    o <- o + 1
    p <- [o]

say_fahrenheit:
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_start + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+convert_decimal + p
    c <- b
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    c <- @+degF + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_end + p
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
    [o] <- p + 2 ; o <- o - 1 ; p <- @+umod + p

    [h] <- b + '0'

    c <- g
    d <- 10
    [o] <- p + 2 ; o <- o - 1 ; p <- @+udiv + p
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
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_start + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+say_end + p
    o <- o + 1
    p <- [o]

say_start:
    [o] <- c ; o <- o - 1
    c <- @+talk + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 2
    c <- [o - (1 + 0)]
    p <- [o]

say_end:
    [o] <- c ; o <- o - 1
    c <- @+rn + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
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
    b <- [@SERIAL]
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

