#include "vga.th"

#define THRESHOLD 64

// The operations later only work if INSET_{ROW,COL}S end up to be a power of 2
#define OFFSET_ROWS 4
#define OFFSET_COLS 8

#define INSET_ROWS (ROWS - (OFFSET_ROWS * 2))
#define INSET_COLS (COLS - (OFFSET_COLS * 2))

.global databuf
databuf: .zero (INSET_ROWS * INSET_COLS)
