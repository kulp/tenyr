// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the condition register

.set VGA_ROWS, 32
.set VGA_COLS, 64

_start:
    o <- ((1 << 13) - 1)
    b <- 0              // indicate non-completion to testbench
    c <- 1              // argument

restart:
    j <- 0              // multiplier
    k <- 0              // multiplicand
    g <- 2              // screen columns consumed

    c <- 0b0100         // decimal point between J and K
    c -> [0x101]

    // write the top row first
loop_top:
    f <- 4              // field width
    n <- k <= (0x100 / (@VGA_ROWS - 2))
    f <- f + n          // conditionally shrink field width
    e <- k + g          // column is multiplicand + filled width
    d <- 0              // row is 0
    c <- k              // number to print is multiplicand
    [o] <- p + 2 ; o <- o - 1 ; p <- @+putnum + p
    k <- k + 1
    g <- g + f - 1
    c <- k <= 16
    p <- @+loop_top & c + p

    // outer loop is j (multiplier, corresponds to row)
loop_j:
    k <- 0
    g <- 0
    // print row header
    f <- 2              // field width
    e <- 0              // column is 0
    d <- j + 1          // row (J + 1)
    c <- j
    [o] <- p + 2 ; o <- o - 1 ; p <- @+putnum + p
    g <- g + f

loop_k:
    f <- 4              // field width
    n <- k <= (0x100 / (@VGA_ROWS - 2))
    f <- f + n          // conditionally shrink field width
    e <- k + g          // column is multiplicand + filled width
    d <- j + 1
    c <- j * k
    [o] <- p + 2 ; o <- o - 1 ; p <- @+putnum + p
    c <- 0x100          // write {J,K} to LEDs
    [c] <- j << 8 + k
    k <- k + 1
    g <- g + f - 1
    c <- k <= 16
    p <- @+loop_k & c + p

    j <- j + 1          // increment N
    c <- j < @VGA_ROWS
    p <- @+loop_j & c + p
    b <- -1             // indicate completion to testbench

    //p <- p + @+restart  // restartable, but exits to testbench by default
    illegal

