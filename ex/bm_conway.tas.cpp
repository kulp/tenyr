#include "common.th"
#include "vga.th"

#define THRESHOLD 64

_start:
    prologue

    // m is generation
    m <- 0

    // i is global video base
    i <- @VGA_BASE
    // g is global flip buffer
    g <- @+databuf + p
    // h is global flip buffer bit index
    h <- 0

    c <- i
    d <- 0xa // in our font, a 50% greyscale
    push(p + 2); p <- @+blank_area + p

    push(p + 2); p <- @+randomise_board + p
    push(p + 2); p <- @+flip + p

forever:
    m -> [0x100]

    push(p + 2); p <- @+compute + p
    push(p + 2); p <- @+flip + p

    m <- m + 1
    p <- p + @+forever

blank_area:
    o <- o - 2
    j -> [o + (2 - 0)]
    n -> [o + (2 - 1)]
    j <- COLS
    j <- j * ROWS
blank_area_loop:
    j <- j - 1
    d -> [c + j]
    n <- j > 0
    p <- @+blank_area_loop & n + p
    o <- o + 3
    n <- [o - (1 + 1)]
    j <- [o - (1 + 0)]
    p <- [o]

randomise_board:
    o <- o - 6
    j -> [o + (6 - 0)]
    k -> [o + (6 - 1)]
    c -> [o + (6 - 2)]
    d -> [o + (6 - 3)]
    l -> [o + (6 - 4)]
    n -> [o + (6 - 5)]
    c <- [@+seed + p]
    push(p + 2); p <- @+srand + p
    j <- (ROWS - 1)
randomise_board_rows:
    k <- (COLS - 1)
randomise_board_cols:
    push(p + 2); p <- @+rand + p
    b <- b >> 23

    l <- b >= THRESHOLD

    b <- j * COLS + k
    l -> [b + g] // write to buf0

    k <- k - 1
    n <- k >= 0
    p <- @+randomise_board_cols & n + p
    j <- j - 1
    n <- j >= 0
    p <- @+randomise_board_rows & n + p
    o <- o + 7
    n <- [o - (1 + 5)]
    l <- [o - (1 + 4)]
    d <- [o - (1 + 3)]
    c <- [o - (1 + 2)]
    k <- [o - (1 + 1)]
    j <- [o - (1 + 0)]
    p <- [o]

flip:
    o <- o - 3
    j -> [o + (3 - 0)]
    k -> [o + (3 - 1)]
    n -> [o + (3 - 2)]
    j <- (ROWS - 1)
flip_rows:
    k <- (COLS - 1)
flip_cols:
    b <- j * COLS + k
    b <- [b + g] // read from buf0

    b <- b @ h

    n <- 'o' &  b
    b <- ' ' &~ b
    n <- b | n

    // convert from inset coordinates to full coordinates
    b <- j * COLS + k
    n -> [b + i] // write to display

    k <- k - 1
    n <- k >= 0
    p <- @+flip_cols & n + p
    j <- j - 1
    n <- j >= 0
    p <- @+flip_rows & n + p

    h <- h ^ 1
    o <- o + 4
    n <- [o - (1 + 2)]
    k <- [o - (1 + 1)]
    j <- [o - (1 + 0)]
    p <- [o]

compute:
    o <- o - 7
    c -> [o + (7 - 0)]
    d -> [o + (7 - 1)]
    e -> [o + (7 - 2)]
    f -> [o + (7 - 3)]
    j -> [o + (7 - 4)]
    k -> [o + (7 - 5)]
    l -> [o + (7 - 6)]
    j <- (ROWS - 1)
compute_rows:
    k <- (COLS - 1)
compute_cols:

    c <- j
    d <- k
    push(p + 2); p <- @+get_neighbour_count + p
    e <- b

    b <- j * COLS + k
    f <- [b + g] // read from buf1
    // E is neighbour count, F is current state
    c <- h ^ 1
    n <- f @ c
    n <- - n
    // N is 1 if alive
    e <- e | n // tricky
    e <- e == 3
    n <- 1 << h
    e <- e & n
    f <- f &~ n
    f <- f |  e
    f -> [b + g] // write to buf0

    k <- k - 1
    n <- k >= 0
    p <- @+compute_cols & n + p
    j <- j - 1
    n <- j >= 0
    p <- @+compute_rows & n + p
    o <- o + 8
    l <- [o - (1 + 6)]
    k <- [o - (1 + 5)]
    j <- [o - (1 + 4)]
    f <- [o - (1 + 3)]
    e <- [o - (1 + 2)]
    d <- [o - (1 + 1)]
    c <- [o - (1 + 0)]
    p <- [o]

get_neighbour_count:
    o <- o - 7
    e -> [o + (7 - 0)]
    f -> [o + (7 - 1)]
    j -> [o + (7 - 2)]
    k -> [o + (7 - 3)]
    l -> [o + (7 - 4)]
    m -> [o + (7 - 5)]
    n -> [o + (7 - 6)]
    m <- 0
    j <- c
    k <- d

    e <- -1
neighbour_loop_rows:
    f <- -1
neighbour_loop_cols:
    // essentially doing `(a + 31) % 32` to get `a - 1` safely
    c <- j + e + ROWS ; c <- c & (ROWS - 1)
    d <- k + f + COLS ; d <- d & (COLS - 1)

    b <- c * COLS + d
    l <- [b + g]
    n <- e | f
    n <- n == 0
    // skip case where e == f == 0 (self)

    c <- h ^ 1
    l <- l @ c ; l <- l &~ n

    m <- m - l

    f <- f + 1
    n <- f <= 1
    p <- @+neighbour_loop_cols & n + p
    e <- e + 1
    n <- e <= 1
    p <- @+neighbour_loop_rows & n + p

    b <- m
    o <- o + 8
    n <- [o - (1 + 6)]
    m <- [o - (1 + 5)]
    l <- [o - (1 + 4)]
    k <- [o - (1 + 3)]
    j <- [o - (1 + 2)]
    f <- [o - (1 + 1)]
    e <- [o - (1 + 0)]
    p <- [o]

seed: .word 0x99999999

