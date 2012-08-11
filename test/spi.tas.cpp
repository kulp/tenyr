#include "common.th"
#include "spi.th"

    prologue

    b <- 0
    b -> [(SPI_BASE + 0x14)] // set divider
    c <- 1
    c -> [(SPI_BASE + 0x18)] // set SS to 1

    b <- rel(IDLE_COMMAND)
    c <- 8
    call(put_spi)

wait_for_sd_ready:
    b <- rel(RESET_COMMAND)
    c <- 8 // number of bits expected in response
    call(put_spi)
    b <- b & 1  // check bottom bit
    b <- b <> 0
    // if the bottom bit was one (busy), b will be true
    jnzrel(b,wait_for_sd_ready)

    illegal

    .global put_spi
put_spi:
    // argument b is address of most significant word of two 32-bit words
    // containing 56 bits (48 command bits + 8 response bits)
    // argument c is number of bits expected as response
    // result b is response word
    push(d)
    push(e)

    d <- [b + 1]
    d -> [(SPI_BASE + 0x0)] // bits 0 - 31
    d <- [b + 0]
    d -> [(SPI_BASE + 0x4)] // bits 32 - 56

    d <- 1
    d <- d << 13 // ASS mode
    d <- d | c + 48
    d -> [(SPI_BASE + 0x10)]

    // GO_BSY
    d <- d | (1 << 8)
    d -> [(SPI_BASE + 0x10)]

L_put_spi_clock_wait:
    // wait for GO_BSY-bit to clear
    e <- [(SPI_BASE + 0x10)]
    e <- e & (1 << 8)
    e <- e <> 0
    jnzrel(e,L_put_spi_clock_wait)

    b <- [(SPI_BASE + 0x0)] // read data
    // chop upper bits
    d <- -1
    d <- d << 8
    d <- ~ d
    b <- b & d

    pop(e)
    pop(d)

    ret

//                      01  |cmd-|  |cmd-argument------    ------------------|  |CRC--| 1   |rsp ---|
IDLE_COMMAND:   .word 0b01__000000__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111
RESET_COMMAND:  .word 0b01__000001__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111

