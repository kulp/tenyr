// This file doesn't adhere to the standard EABI !

#include "common.th"
#include "vga.th"

.set SIZEOF_SNAKE, 4

_start:
    prologue

    call(init_display)

#ifdef TEST
    c <- rel(_start) ; call(srand) # seed based on where we are loaded
#endif

    n <- rel(snakes)
    e <- rel(snakes_after)
init:
    call(rand)
    j <- b >>> 27 // 5 bits
    k <- b >>> 21 ; k <- k & 0x3f // 6 bits
    j -> [n + 2]
    k -> [n + 3]

    n <- n + $SIZEOF_SNAKE
    j <- n < e
    jnzrel(j,init)

    g <- -1 // constant
L_loop:
    n <- rel(snakes)
L_snake:
    // j:k = row:col
    j <- [n + 2]
    k <- [n + 3]

    // direction and character choice are not independent variables
    call(rand)
    f <- b // save away random value for later
    d <- f >>> 30 // a direction 0 - 3 ; clockwise, 0 = up

    // start get_incrs
    // direction map
    // 0 -> -1  0
    // 1 ->  0  1
    // 2 ->  1  0
    // 3 ->  0 -1

    h <- d & 1 + g // 1 -> 0, 0 -> -1
    i <- d & 2 + g // 2 -> 1, 0 -> -1

    b <- i & h
    c <- -i
    c <- c &~ h

    // end get_incrs

    j <- j + b
    k <- k + c

    // start clamp
    // Check for out-of-bounds and bounce off the wall
    b <- j >= ROWS  ; j <- b << 1 + j
    b <- j >= a + 1 ; j <- b << 1 + j
    c <- k >= COLS  ; k <- c << 1 + k
    c <- k >= a + 1 ; k <- c << 1 + k
    // end clamp

    j -> [n + 2] // store new row, col
    k -> [n + 3]

    i <- [n + 1]
    c <- f >>> i
    h <- i == 0 // special case when shift-value is zero : no randomness
    c <- c &~ h
    h <- [n + 0]
    c <- c + h
    d <- j * COLS + k   // d is offset into display
    e <- 0x100
    e <- e << 8         // e is video base
    c -> [e + d]        // e is d characters past start of text region

    n <- n + $SIZEOF_SNAKE
    e <- rel(snakes_after)
    j <- n < e
    jnzrel(j,L_snake)

	j <- 250
	j <- j * 75
slow_down:
	e <- j > 0
	j <- j - 1
	jnzrel(e,slow_down)

    goto(L_loop) // infinite loop

snakes:
    // base type, shift distance for rand(), row, col
    .word 'A', 28, 0, 0;
    .word '0', 29, 0, 0;
    .word '-', 31, 0, 0;
    .word ':', 31, 0, 0;
    .word '*', 31, 0, 0;
    .word '(', 31, 0, 0;

    .word ' ',  0, 0, 0; // eraser snakes
    .word ' ',  0, 0, 0;
    .word ' ',  0, 0, 0;
    .word ' ',  0, 0, 0;
    .word ' ',  0, 0, 0;
    .word ' ',  0, 0, 0;
snakes_after: .word 0

