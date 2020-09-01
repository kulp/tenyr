#define PRINT 1

_start:
    o <- ((1 << 13) - 1)

#if 1
    c <- 0
    d <- 0x234
    call(imul)
    // 0x0 * 0x234 = 0x0
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- 0x7ed
    d <- 0
    call(imul)
    // 0x7ed * 0x0 = 0x0
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- 0x7ed
    d <- 0x234
    call(imul)
    // 0x7ed * 0x234 = 0x117624
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- -1024
    d <- 1023
    call(imul)
    // -1024 * 1023 = -1047552 = 0xfff00400
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- -2048
    d <- 2047
    call(imul)
    // -2048 * 2047 = -4192256 = 0xffc00800
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- 3
    d <- -2
    call(imul)
    // 3 * -2 = 0xfffffffa
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- -3
    d <- -2
    call(imul)
    // -3 * -2 = 0x6
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif

#if 1
    c <- -3
    d <- 2
    call(imul)
    // -3 * 2 = 0xfffffffa
#if PRINT
    c <- b ; call(print_hex)
#endif
#endif
    illegal

print_hex:
    [o] <- c ; o <- o - 1
    c <- @+_0x + p
    call(puts)
    o <- o + 1 ; c <- [o]
    call(convert_hex)
    c <- b
    call(puts)
    c <- @+_nl + p
    call(puts)
    o <- o + 1
    p <- [o]

_0x: .chars "0x" ; .word 0
_nl: .word '\n' ; .word 0

