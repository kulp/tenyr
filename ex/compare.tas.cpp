_start:
    o <- ((1 << 13) - 1)

    c <- @+this + p
    d <- @+that + p
    f <- @+strcmp + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+cmp + p

    c <- @+this + p
    d <- @+this + p
    f <- @+strcmp + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+cmp + p

    c <- @+this + p
    d <- @+that + p
    e <- 6
    f <- @+strncmp + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+cmp + p

    c <- @+this + p
    d <- @+that + p
    e <- 50
    f <- @+strncmp + p
    [o] <- p + 2 ; o <- o - 1 ; p <- @+cmp + p

    illegal

cmp:
    [o] <- c ; o <- o - 1
    [o] <- d ; o <- o - 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p // output first string
    c <- @+nl + p   // followed by newline
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1 ; d <- [o]
    o <- o + 1 ; c <- [o]

    [o] <- c ; o <- o - 1
    [o] <- d ; o <- o - 1
    c <- d
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p // output second string
    c <- @+nl + p   // followed by newline
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1 ; d <- [o]
    o <- o + 1 ; c <- [o]

    [o] <- p + 2 ; o <- o - 1 ; p <- @+check + p // actually compare them

    c <- @+nl + p   // ... followed by newline
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    c <- @+nl + p   // and another
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p

    o <- o + 1
    p <- [o]

check:
    [o] <- p + 2 ; o <- o - 1
    p <- f          // call indirect
    p <- @+_mismatched & b + p
    c <- @+_match_message + p
    p <- p + @+_finished
_mismatched:
    c <- @+_mismatch_message + p
_finished:
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    o <- o + 1
    p <- [o]

_match_message:
    .chars "strings matched"
    .word 0
_mismatch_message:
    .chars "strings did not match"
    .word 0

this:
    .chars "this is a longish string"
    .word 0
that:
    .chars "this is a longish string2"
    .word 0
nl:
    .word '\n'
    .word 0

