#include "common.th"

// arguments in C and D
// result in B ; 0 for match, -1 for mismatch

    .global strcmp
strcmp:
    o <- o - 5
    e -> [o + (5 - 0)]
    g -> [o + (5 - 1)]
    h -> [o + (5 - 2)]
    i -> [o + (5 - 3)]
    j -> [o + (5 - 4)]
    b <- 0              // start with matching
strcmp_loop:
    j <- [c]            // load word from string
    g <- [d]            // load word from string
    h <- j == a         // if it is zero, we are done
    i <- g == a         // if it is zero, we are done
    p <- @+strcmp_done & h + p
    p <- @+strcmp_done & i + p
    c <- c + 1          // increment index for next time
    d <- d + 1          // increment index for next time
    e <- j == g         // check for mismatch
    b <- b |~ e         // accumulate mismatches
    p <- p + @+strcmp_loop
strcmp_done:
    e <- j == g         // check for mismatch
    b <- b |~ e         // accumulate mismatches
    e <- h == i         // check for length mismatch
    b <- b |~ e         // accumulate mismatches
    o <- o + 6
    j <- [o - (1 + 4)]
    i <- [o - (1 + 3)]
    h <- [o - (1 + 2)]
    g <- [o - (1 + 1)]
    e <- [o - (1 + 0)]
    p <- [o]

// arguments in C, D, and E
// result in B ; 0 for match, -1 for mismatch

    .global strncmp
strncmp:
    o <- o - 5
    f -> [o + (5 - 0)]
    g -> [o + (5 - 1)]
    h -> [o + (5 - 2)]
    i -> [o + (5 - 3)]
    j -> [o + (5 - 4)]
    b <- 0              // start with matching
strncmp_loop:
    h <- e < 1          // check length to go
    p <- @+strncmp_nreached & h + p
    j <- [c]            // load word from string
    g <- [d]            // load word from string
    h <- j == a         // if it is zero, we are done
    i <- g == a         // if it is zero, we are done
    p <- @+strncmp_done & h + p
    p <- @+strncmp_done & i + p
    c <- c + 1          // increment index for next time
    d <- d + 1          // increment index for next time
    e <- e - 1          // decrement length to check
    f <- j == g         // check for mismatch
    b <- b |~ f         // accumulate mismatches
    p <- p + @+strncmp_loop
strncmp_done:
    f <- j == g         // check for mismatch
    b <- b |~ f         // accumulate mismatches
    f <- h == i         // check for length mismatch
    b <- b |~ f         // accumulate mismatches
strncmp_nreached:
    o <- o + 6
    j <- [o - (1 + 4)]
    i <- [o - (1 + 3)]
    h <- [o - (1 + 2)]
    g <- [o - (1 + 1)]
    f <- [o - (1 + 0)]
    p <- [o]

