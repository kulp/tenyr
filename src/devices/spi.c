// Simulated implementation of http://opencores.org/project,spi
//  which belongs to Simon Srot <simons@opencores.org>
// Connects tenyr wishbone (plain local bus now, since wishbone is not
// simulated in any special way) to a seekable FILE*.
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"

#define SPI_BASE    0x200
#define SPI_LEN     (0x7 * 4) /* seven registers at four addresses each) */
#define SPI_END     (SPI_BASE + SPI_LEN)

struct spi_state {
    FILE *store;

    union {
        uint32_t raw[7];
        struct {
            union {
                struct {
                    uint32_t Rx0, Rx1, Rx2, Rx3;
                } Rx;
                struct {
                    uint32_t Tx0, Tx1, Tx2, Tx3;
                } Tx;
            } data;
            struct {
                union {
                    unsigned CTRL:13;
                    // Technically bit ordering isn't defined by C99 here
                    struct {
                        unsigned CHAR_LEN   : 7;    // bits [ 6:0]
                        unsigned /* rsvd */ : 1;    // bit  [ 7]
                        unsigned GO_BSY     : 1;    // bit  [ 8]
                        unsigned Rx_NEG     : 1;    // bit  [ 9]
                        unsigned Tx_NEG     : 1;    // bit  [10]
                        unsigned LSB        : 1;    // bit  [11]
                        unsigned IE         : 1;    // bit  [12]
                        unsigned ASS        : 1;    // bit  [13]
                        unsigned /* rsvd */ :18;    // bits [31:14]
                    } bits;
                } u;
            } ctrl;
        } fmt;
    } regs;
};

static int spi_init(struct sim_state *s, void *cookie, ...)
{
    struct spi_state *spi = *(void**)cookie = malloc(sizeof *spi);

    return 0;
}

static int spi_fini(struct sim_state *s, void *cookie)
{
    struct spi_state *spi = cookie;

    free(spi);

    return 0;
}

static int spi_op(struct sim_state *s, void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct spi_state *spi = cookie;
    (void)spi;
    assert(("Address within address space", !(addr & ~PTR_MASK)));

    return 0;
}

int spi_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { SPI_BASE, SPI_END },
        .op = spi_op,
        .init = spi_init,
        .fini = spi_fini,
    };

    return 0;
}

