#include "common.th"
#include "spi.th"

    prologue

    b <- 0
    b -> [(SPI_BASE + 0x14)]    // set divider

    b <- rel(IDLE_COMMAND)      // address of command
    c <- 0                      // device index 0
    d <- 8                      // number of bits expected in response
    call(put_spi)

wait_for_sd_ready:
    b <- rel(RESET_COMMAND)     // address of command
    d <- 8                      // number of bits expected in response
    call(put_spi)
    b <- b & 1                  // mask off all but idle bit
    b <- b <> 0                 // check if set
    jnzrel(b,wait_for_sd_ready) // loop if idle bit is set (means busy)

    illegal

//------------------------------------------------------------------------------
    .global put_spi
put_spi:
    // argument b is address of most significant word of two 32-bit words
    // containing 56 bits (48 command bits + 8 response bits)
    // argument c is device number
    // argument d is number of bits expected as response
    // result b is response word
    push(e)                     // use e as scratch

    push(b)                     // save argument b which is clobbered by ipow2
    call(ipow2)                 // convert index in c to mask in b
    c <- b                      // move mask in b to c
    pop(b)                      // restore argument b

    c -> [(SPI_BASE + 0x18)]    // set SS bit to appropriate device mask

    e <- [b + 1]
    e -> [(SPI_BASE + 0x0)]     // bits 0 - 31
    e <- [b + 0]
    e -> [(SPI_BASE + 0x4)]     // bits 32 - 56

    e <- 1
    e <- e << 13                // automatic slave selection mode
    e <- e | d + 48             // command length is 48 bits
    e -> [(SPI_BASE + 0x10)]    // write control register

    e <- e | (1 << 8)           // set GO_BSY bit
    e -> [(SPI_BASE + 0x10)]    // write control register again

L_put_spi_wait:                 // wait for GO_BSY-bit to clear
    c <- [(SPI_BASE + 0x10)]    // read control register
    c <- c & (1 << 8)           // mask off all but GO_BSY bit
    c <- c <> 0                 // check if set
    jnzrel(c,L_put_spi_wait)    // loop if set

    b <- [(SPI_BASE + 0x0)]     // read response byte
    b <- b & 0xff               // chop upper bits

    pop(e)

    ret

//                      01  |cmd-|  |cmd-argument------    ------------------|  |CRC--| 1   |rsp ---|
IDLE_COMMAND:   .word 0b01__000000__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111
RESET_COMMAND:  .word 0b01__000001__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111

