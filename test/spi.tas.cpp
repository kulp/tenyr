#define nop a <- a;
#define SPI_BASE 0x200
a -> [(SPI_BASE + 0x14)] // set divider to zero
b <- 1
b -> [(SPI_BASE + 0x18)] // set SS to 1
// data
b <- 0x1b
b -> [(SPI_BASE + 0)]
// length
b <- 8
b -> [(SPI_BASE + 0x10)]
// GO_BSY
b <- b | (1 << 8)
b -> [(SPI_BASE + 0x10)]
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
illegal
