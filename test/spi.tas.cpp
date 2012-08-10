#include "common.th"

#define nop a <- a;
#define SPI_BASE 0x200
b <- 0
b -> [(SPI_BASE + 0x14)] // set divider
// data
b <- [reloff(IDLE_COMMAND, 1)]
b -> [(SPI_BASE + 0x0)] // bits 0 - 31
b <- [reloff(IDLE_COMMAND, 0)]
b -> [(SPI_BASE + 0x4)] // bits 32 - 47

b <- (48 + 8) // message length +  response length

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

IDLE_COMMAND: // includes response ones
	.word 0x400000, 0x000001ff
