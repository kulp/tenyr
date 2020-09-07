#define PRINT 1

_start:
    o <- ((1 << 13) - 1)

    c <- 0
    d <- 0x234
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // 0x0 * 0x234 = 0x0
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- 0x7ed
    d <- 0
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // 0x7ed * 0x0 = 0x0
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- 0x7ed
    d <- 0x234
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // 0x7ed * 0x234 = 0x117624
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- -1024
    d <- 1023
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // -1024 * 1023 = -1047552 = 0xfff00400
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- -2048
    d <- 2047
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // -2048 * 2047 = -4192256 = 0xffc00800
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- 3
    d <- -2
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // 3 * -2 = 0xfffffffa
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- -3
    d <- -2
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // -3 * -2 = 0x6
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p

    c <- -3
    d <- 2
    [o] <- p + 2 ; o <- o - 1 ; p <- @+imul + p
    // -3 * 2 = 0xfffffffa
    c <- b ; [o] <- p + 2 ; o <- o - 1 ; p <- @+print_hex + p
    illegal

print_hex:
    [o] <- c ; o <- o - 1
    c <- @+_0x + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1 ; c <- [o]
    [o] <- p + 2 ; o <- o - 1 ; p <- @+convert_hex + p
    c <- b
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    c <- @+_nl + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1
    p <- [o]

_0x: .chars "0x" ; .word 0
_nl: .word '\n' ; .word 0

