#include "common.th"
#include "vga.th"

#define THRESHOLD 64

// The operations later only work if INSET_{ROW,COL}S end up to be a power of 2
#define OFFSET_ROWS 4
#define OFFSET_COLS 8

#define INSET_ROWS (ROWS - (OFFSET_ROWS * 2))
#define INSET_COLS (COLS - (OFFSET_COLS * 2))

_start:
    bare_metal_init()   // TODO this shouldn't be necessary
    prologue

    call(init_display)
    call(disable_cursor)

    // m is generation
    m <- 0

    // i is global video base
    i <- VGA_BASE
    // g is global flip buffer
    g <- rel(databuf)
    // h is global flip buffer bit index
    h <- 0

    c <- i
    d <- 0xa // in our font, a 50% greyscale
    call(blank_area)

    call(randomise_board)
    call(flip)

forever:
    m -> [0x100]

    call(compute)
    call(flip)

    m <- m + 1
    goto(forever)

blank_area:
    pushall(j,n)
    j <- COLS
    j <- j * ROWS
blank_area_loop:
    j <- j - 1
    d -> [c + j]
    n <- j > 0
    jnzrel(n,blank_area_loop)
    popall_ret(j,n)

randomise_board:
    pushall(j,k,c,d,l,n)
    c <- [rel(seed)]
    call(srand)
    j <- (INSET_ROWS - 1)
randomise_board_rows:
    k <- (INSET_COLS - 1)
randomise_board_cols:
    call(rand)
    b <- b >> 23

    l <- b >= THRESHOLD

    b <- j * INSET_COLS + k
    l -> [b + g] // write to buf0

    k <- k - 1
    n <- k >= 0
    jnzrel(n,randomise_board_cols)
    j <- j - 1
    n <- j >= 0
    jnzrel(n,randomise_board_rows)
    popall_ret(j,k,c,d,l,n)

flip:
    pushall(j,k,n)
    j <- (INSET_ROWS - 1)
flip_rows:
    k <- (INSET_COLS - 1)
flip_cols:
    b <- j * INSET_COLS + k
    b <- [b + g] // read from buf0

    b <- b @ h

    n <- 'o' &  b
    b <- ' ' &~ b
    n <- b | n

    // convert from inset coordinates to full coordinates
    b <- j + OFFSET_ROWS
    b <- b * COLS + k
    b <- b + OFFSET_COLS
    n -> [b + i] // write to display

    k <- k - 1
    n <- k >= 0
    jnzrel(n,flip_cols)
    j <- j - 1
    n <- j >= 0
    jnzrel(n,flip_rows)

    h <- h ^ 1
    popall_ret(j,k,n)

compute:
    pushall(c,d,e,f,j,k,l)
    j <- (INSET_ROWS - 1)
compute_rows:
    k <- (INSET_COLS - 1)
compute_cols:

    c <- j
    d <- k
    call(get_neighbour_count)
    e <- b

    b <- j * INSET_COLS + k
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
    jnzrel(n,compute_cols)
    j <- j - 1
    n <- j >= 0
    jnzrel(n,compute_rows)
    popall_ret(c,d,e,f,j,k,l)

get_neighbour_count:
    pushall(e,f,j,k,l,m,n)
    m <- 0
    j <- c
    k <- d

    e <- -1
neighbour_loop_rows:
    f <- -1
neighbour_loop_cols:
    // essentially doing `(a + 31) % 32` to get `a - 1` safely
    c <- j + e + INSET_ROWS ; c <- c & (INSET_ROWS - 1)
    d <- k + f + INSET_COLS ; d <- d & (INSET_COLS - 1)

    b <- c * INSET_COLS + d
    l <- [b + g]
    n <- e | f
    n <- n == 0
    // skip case where e == f == 0 (self)

    c <- h ^ 1
    l <- l @ c ; l <- l &~ n

    m <- m - l

    f <- f + 1
    n <- f <= 1
    jnzrel(n,neighbour_loop_cols)
    e <- e + 1
    n <- e <= 1
    jnzrel(n,neighbour_loop_rows)

    b <- m
    popall_ret(e,f,j,k,l,m,n)

seed: .word 0x99999999

