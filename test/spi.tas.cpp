#include "common.th"

#define nop a <- a;
#define SPI_BASE 0x200
b <- 0
b -> [(SPI_BASE + 0x14)] // set divider
// data
b <- 0
b <- b | 0      // bits 20 - 31
b <- b << 12
b <- b | 0      // bits 8 - 19
b <- b << 8
b <- b | 1      // bits 0 - 7
b -> [(SPI_BASE + 0x0)] // bits 0 - 31
b <- 0
b <- b | 0x400  // bits 32 - 43
b <- b << 4
b <- b | 0      // bits 44 - 47
b -> [(SPI_BASE + 0x4)] // bits 32 - 47

b <- 48 // length

//b <- b | (1 << 11) // LSB mode


d <- 1
d <- d << 13
b <- b | d // ASS mode
b -> [(SPI_BASE + 0x10)]

c <- 1
c -> [(SPI_BASE + 0x18)] // set SS to 1
// GO_BSY
b <- b | (1 << 8)
b -> [(SPI_BASE + 0x10)]

d <- 50 # wait count
loop:
d <- d - 1
e <- d <> 0
jnzrel(e,loop)

b <- [(SPI_BASE + 0x0)] // read data
c <- [(SPI_BASE + 0x4)] // read data
illegal
