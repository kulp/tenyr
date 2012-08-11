#include "common.th"
#include "spi.th"

    prologue

    b <- 0
    b -> [(SPI_BASE + 0x14)] // set divider
    c <- 1
    c -> [(SPI_BASE + 0x18)] // set SS to 1

    b <- rel(IDLE_COMMAND)
    call(put_spi)

    c <- [(SPI_BASE + 0x0)] // read data
    b <- [(SPI_BASE + 0x4)] // read data
    illegal

put_spi:
    e <- [b + 1]
    e -> [(SPI_BASE + 0x0)] // bits 0 - 31
    e <- [b + 0]
    e -> [(SPI_BASE + 0x4)] // bits 32 - 47

    k <- (48 + 8) // message length +  response length

    //e <- e | (1 << 11) // LSB mode

    d <- 1
    d <- d << 13
    e <- k | d // ASS mode
    e -> [(SPI_BASE + 0x10)]

    // GO_BSY
    e <- e | (1 << 8)
    e -> [(SPI_BASE + 0x10)]

    d <- k # wait count
    loop:
    d <- d - 1
    e <- d <> 0
    jnzrel(e,loop)

    ret

IDLE_COMMAND:
    /*      01  |cmd-|  |cmd-argument------    ------------------|  |CRC--| 1   |rsp ---| */
    .word 0b01__000000__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111

RESET_COMMAND:
    /*      01  |cmd-|  |cmd-argument------    ------------------|  |CRC--| 1   |rsp ---| */
    .word 0b01__000001__0000_0000_0000_0000, 0b0000_0000_0000_0000__0000000_1___1111_1111

