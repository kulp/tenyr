#define nop a <- a;
#define SPI_BASE 0x200
b <- 0
b -> [(SPI_BASE + 0x14)] // set divider
b <- 1
b -> [(SPI_BASE + 0x18)] // set SS to 1
// data
b <- 0
b <- b | 0x123
b <- b << 12
b <- b | 0x456
b <- b << 8
b <- b | 0x78
b -> [(SPI_BASE + 0x0)]
b <- 1
b -> [(SPI_BASE + 0x4)]
// length
b <- 33
b -> [(SPI_BASE + 0x10)]
// GO_BSY
b <- b | (1 << 8)
b -> [(SPI_BASE + 0x10)]
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops

nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops

nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops

#if 0
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops

nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
nop nop nop nop nop nop nop nop // 8 nops
#endif

b <- [(SPI_BASE + 0x0)]	// read data
c <- [(SPI_BASE + 0x4)]	// read data
illegal
