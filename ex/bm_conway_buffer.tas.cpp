.set VGA_ROWS, 32
.set VGA_COLS, 64

#define THRESHOLD 64

// The operations later only work if INSET_{ROW,COL}S end up to be a power of 2
.set OFFSET_ROWS, 4
.set OFFSET_COLS, 8

// Due to intentional limitations on @-expressions, we cannot actually compute
// some of the expressions shown below.
//.set INSET_ROWS, (@VGA_ROWS - (@OFFSET_ROWS * 2))
//.set INSET_COLS, (@VGA_COLS - (@OFFSET_COLS * 2))

.global databuf
// databuf: .zero (@INSET_ROWS * @INSET_COLS)
databuf: .zero ((32 - (4 * 2)) * (64 - (8 * 2)))
