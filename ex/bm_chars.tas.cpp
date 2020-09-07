.set VGA_ROWS, 32
.set VGA_COLS, 64
.set VGA_BASE, 0x10000

_start:
    o <- ((1 << 13) - 1)
    b <- 0              // indicate non-completion to testbench

restart:
    c <- 0
    j <- @VGA_BASE
    k <- @VGA_ROWS; k <- k * @VGA_COLS
    k <- k + j

top:
    c -> [j]
    c <- c + 1
    c <- c & 0xff

    j <- j + 1
    n <- j < k
    p <- @+top & n + p

    b <- -1             // indicate completion to testbench
    //p <- p + @+restart  // restartable, but exits to testbench by default
    illegal

