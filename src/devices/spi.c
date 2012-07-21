// Simulated implementation of http://opencores.org/project,spi
//  which belongs to Simon Srot <simons@opencores.org>
// Connects tenyr wishbone (plain local bus now, since wishbone is not
// simulated in any special way) to a spi_ops implementation. If param
// "spi.impl" is set, a spi_ops implementation with that stem name is loaded
// using dlsym(). Otherwise, acts as if nothing is attached to the SPI pins.

// _GNU_SOURCE is needed for RTLD_DEFAULT on GNU/Linux, although that flag
// works on apple-darwin as well
#define _GNU_SOURCE 1

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "common.h"
#include "device.h"
#include "spi.h"

#define FPTR_FROM_VPTR(Type,Expr) \
    *(Type * MAY_ALIAS *)&(Expr)

#define SPI_BASE    0x200
#define SPI_LEN     (0x7 * 4) /* seven registers at four addresses each) */
#define SPI_END     (SPI_BASE + SPI_LEN - 1)

#define NINST       8 /* number of instances connectable */

#define SPI_INIT_CYCLES 2 /* why 2 ? XXX */

struct spi_state {
    struct spi_ops impls[NINST];
    void *impl_cookies[NINST];
    enum {
        SPI_EMU_RESET,
        SPI_EMU_SELECTED,
        SPI_EMU_BUSY,
        SPI_EMU_DONE
    } state;
    unsigned dividend;  // how far into division in wishbone cycles
    unsigned cyc;       // how far into transaction in SPI cycles
    unsigned remaining; // how many bits remain to be transferred

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
            unsigned DIVIDER    :16;
            unsigned            :16;
            unsigned SS         :NINST;
            unsigned            :(32 - NINST);
        } fmt;
    } regs;
};

static void spi_reset_defaults(struct spi_state *spi)
{
    spi->regs.fmt.data.Rx.Rx0 = 0x00000000;
    spi->regs.fmt.data.Rx.Rx1 = 0x00000000;
    spi->regs.fmt.data.Rx.Rx2 = 0x00000000;
    spi->regs.fmt.data.Rx.Rx3 = 0x00000000;

    spi->regs.fmt.data.Tx.Tx0 = 0x00000000;
    spi->regs.fmt.data.Tx.Tx1 = 0x00000000;
    spi->regs.fmt.data.Tx.Tx2 = 0x00000000;
    spi->regs.fmt.data.Tx.Tx3 = 0x00000000;

    spi->regs.fmt.ctrl.u.CTRL = 0x00000000;

    spi->regs.fmt.DIVIDER     = 0x0000ffff;

    spi->regs.fmt.SS          = 0x00000000;
}

static int spi_emu_init(struct sim_state *s, void *cookie, ...)
{
    struct spi_state *spi = *(void**)cookie = malloc(sizeof *spi);

    spi_reset_defaults(spi);

    const char *implname = NULL;
    if (param_get(s, "spi.impl", &implname)) {
        int inst = 0; // TODO support more than one instance

#define GET_CB(Stem)                                                \
        do {                                                        \
            char buf[64];                                           \
            void *ptr = dlsym(RTLD_DEFAULT, buf);                   \
            snprintf(buf, sizeof buf, "%s_spi_"#Stem, implname);    \
            spi->impls[inst].Stem = FPTR_FROM_VPTR(spi_##Stem,ptr); \
        } while (0)                                                 \
        //

        GET_CB(clock);
        GET_CB(init);
        GET_CB(select);
        GET_CB(fini);

        if (!spi->impls[inst].clock)
            fatal(PRINT_ERRNO, "Failed to locate SPI impl '%s'", implname);

        if (spi->impls[inst].init)
            spi->impls[inst].init(&spi->impl_cookies[inst]);
    }

    return 0;
}

static int spi_emu_fini(struct sim_state *s, void *cookie)
{
    struct spi_state *spi = cookie;

    for (int inst = 0; inst < NINST; inst++) {
        if (spi->impls[inst].fini)
            spi->impls[inst].fini(&spi->impl_cookies[inst]);
    }

    free(spi);

    return 0;
}

static int spi_op(struct sim_state *s, void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct spi_state *spi = cookie;
    uint32_t offset = addr - SPI_BASE;

    assert(("Address within address space", !(addr & ~PTR_MASK)));
    assert(("Lower bits of offset are cleared", !(offset & 0x7)));

    // TODO catch writes to GO_BSY

    // "When a transfer is in progress, writing to any register of the SPI
    // Master core has no effect."
    if (spi->state == SPI_EMU_RESET)
        spi->regs.raw[offset >> 3] = *data;

    return 0;
}

static int spi_emu_cycle(struct sim_state *s, void *cookie)
{
    struct spi_state *spi = cookie;
    if (spi->dividend++ >= (unsigned)((spi->regs.fmt.DIVIDER + 1) * 2 - 1)) {
        int push = spi->regs.fmt.data.Tx.Tx0 & 1;
        uint32_t SS = spi->regs.fmt.SS;
        assert(("SPI slave select is one-hot", ((SS ^ (SS - 1)) == 0)));

        for (int inst = 0; inst < NINST; inst++) {
            struct spi_ops *ops = &spi->impls[inst];
            void *cookie = spi->impl_cookies[inst];
            int pull = -1;

            // a device not having a clock() cb is invalid or empty
            if (!ops->clock)
                continue; // clock() is the only required callback

            // check if we are selected
            if (SS & (1 << inst)) {
                switch (spi->state) {
                    case SPI_EMU_RESET:
                        if (ops->select)
                            ops->select(cookie, 1);
                        break;
                    case SPI_EMU_DONE:
                        if (ops->select)
                            ops->select(cookie, 0);
                        break;
                    case SPI_EMU_SELECTED:
                    case SPI_EMU_BUSY: // XXX why for both SELECTED and BUSY
                        ops->clock(cookie, 1, push, &pull);
                        break;
                    default:
                        fatal(0, "Invalid SPI master state");
                }
            } else {
                switch (spi->state) {
                    case SPI_EMU_SELECTED: {
                        int pull = -1;
                        ops->clock(cookie, 0, push, &pull);
                        assert(("Inactive SPI slave does not generate traffic", (pull == -1)));
                        break;
                    }
                    default:
                        fatal(0, "Invalid SPI master state");
                }
            }
        }

        switch (spi->state) {
            case SPI_EMU_RESET:
                if (spi->cyc > SPI_INIT_CYCLES) {
                    spi->state = SPI_EMU_SELECTED;
                } else {
                    spi->remaining = spi->regs.fmt.ctrl.u.bits.CHAR_LEN;
                    if (spi->remaining == 0)
                        spi->remaining = 128;
                }
                break;
            case SPI_EMU_SELECTED:
                spi->state = SPI_EMU_BUSY;
                break;
            case SPI_EMU_BUSY:
                if (!spi->remaining--)
                    spi->state = SPI_EMU_DONE;
                break;
            case SPI_EMU_DONE:
                spi->state = SPI_EMU_RESET;
                break;
            default:
                fatal(0, "Invalid SPI master state");
        }

        spi->cyc++;
        spi->dividend = 0;
    }

    return 0;
}

int spi_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { SPI_BASE, SPI_END },
        .op = spi_op,
        .init = spi_emu_init,
        .fini = spi_emu_fini,
        .cycle = spi_emu_cycle,
    };

    return 0;
}

