#define nop a <- a;
#define SPI_BASE 0x200
a -> [(SPI_BASE + 0x14)] # set divider to zero
nop
nop
nop
nop
b <- 0xff
b -> [(SPI_BASE + 0)]
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
illegal
