#include "common.th"
#include "spi.th"

    prologue

    b <- 0
    b -> [(SPI_BASE + 0x14)]    // set divider

    // There's something about 74 cycles minimum delay on init ; where does
    // that happen ?
    b <- rel(CMD_IDLE)          // address of command
    c <- 0                      // device index 0
    d <- 8                      // number of bits expected in response
    call(put_spi)

wait_for_sd_ready:
    b <- rel(CMD_RESET)         // address of command
    c <- 0                      // device index 0
    d <- 8                      // number of bits expected in response
    call(put_spisd)

set_blocklen:
    b <- rel(CMD_SET_BLOCKLEN)  // address of command
    c <- 0                      // device index 0
    d <- 8                      // number of bits expected in response
    call(put_spisd)

    illegal

put_spisd:
    pushall(b,c,d)              // command addr, device index, response bits
    call(put_spi)
    b <- b & 1                  // mask off all but idle bit
    e <- b <> 0                 // check if set
    popall(b,c,d)
    jnzrel(e,put_spisd)         // loop if idle bit is set (means busy)
    ret

//------------------------------------------------------------------------------
    .global put_spi
put_spi:
    // argument b is address of most significant word of two 32-bit words
    // containing 48 + d bits (48 command bits + 8 or 16 response bits)
    // argument c is device number
    // argument d is number of bits expected as response
    // result b is response word

    push(b)                     // need a scratch ; use b
    b <- 1
    b <- b << c
    b -> [(SPI_BASE + 0x18)]    // set SS bit to appropriate device mask
    pop(b)                      // restore argument b

    c <- [b + 0]
    c -> [(SPI_BASE + 0x4)]     // bits 56 (MSB) - 32
    c <- [b + 1]
    c -> [(SPI_BASE + 0x0)]     // bits 31 - 0 (LSB)

    c <- 1
    c <- c << 13                // automatic slave selection mode
    c <- c | d + 48             // command length is 48 bits
    c -> [(SPI_BASE + 0x10)]    // write control register

    c <- c | (1 << 8)           // set GO_BSY bit
    c -> [(SPI_BASE + 0x10)]    // write control register again

L_put_spi_wait:                 // wait for GO_BSY-bit to clear
    c <- [(SPI_BASE + 0x10)]    // read control register
    c <- c & (1 << 8)           // mask off all but GO_BSY bit
    c <- c <> 0                 // check if set
    jnzrel(c,L_put_spi_wait)    // loop if set

    b <- [(SPI_BASE + 0x0)]     // read response byte
    b <- b & 0xff               // chop upper bits

    ret

//                          01  |cmd-|  |cmd-argument------    ------------------|  |CRC--| 1   |rsp ---|
CMD_IDLE:           .word 0b01__000000__0000_0000_0000_0000, 0b0000_0000_0000_0000__1001010_1___1111_1111
CMD_RESET:          .word 0b01__000001__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111
CMD_SET_BLOCKLEN:   .word 0b01__010000__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111

