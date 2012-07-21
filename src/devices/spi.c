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

#define SPI_INIT_CYCLES 3 /* TODO justify this number */

struct spi_state {
    struct spi_ops impls[NINST];
    void *impl_cookies[NINST];
    enum {
        SPI_EMU_RESET,
        SPI_EMU_STARTED,
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
        char buf[128];
        snprintf(buf, sizeof buf, "lib%s"DYLIB_SUFFIX, implname);
        void *libhandle = dlopen(buf, RTLD_LOCAL);
        if (!libhandle) {
            debug(1, "Could not load %s, trying default library search", buf);
            libhandle = RTLD_DEFAULT;
        }

#define GET_CB(Stem)                                                \
        do {                                                        \
            char buf[64];                                           \
            snprintf(buf, sizeof buf, "%s_spi_"#Stem, implname);    \
            void *ptr = dlsym(libhandle, buf);                      \
            spi->impls[inst].Stem = FPTR_FROM_VPTR(spi_##Stem,ptr); \
        } while (0)                                                 \
        //

        GET_CB(clock);
        if (!spi->impls[inst].clock) {
            const char *err = dlerror();
            if (err)
                fatal(0, "Failed to locate SPI clock cb for '%s' ; %s", implname, err);
            else
                fatal(0, "SPI clock cb for '%s' is NULL ? : %s", implname);
        }

        GET_CB(init);
        GET_CB(select);
        GET_CB(fini);

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
    assert(("Lower bits of offset are cleared", !(offset & 0x3)));

    // TODO catch writes to GO_BSY

    // "When a transfer is in progress, writing to any register of the SPI
    // Master core has no effect."
    if (spi->state == SPI_EMU_RESET) {
        if (offset == 0x10) { // CTRL register
            uint32_t go_mask = 1 << 8;
            uint32_t new_go_bit =  go_mask & *data;
            uint32_t old_go_bit =  go_mask & spi->regs.raw[offset >> 2];
            uint32_t new_others = ~go_mask & *data;
            uint32_t old_others = ~go_mask & spi->regs.raw[offset >> 2];

            if (!((new_go_bit == old_go_bit) || (new_others == old_others)))
                breakpoint("GO_BSY bit changed at the same time as other control bits");

            if (new_go_bit) {
                spi->state = SPI_EMU_STARTED;
                spi->remaining = spi->regs.fmt.ctrl.u.bits.CHAR_LEN;
                if (spi->remaining == 0)
                    spi->remaining = 128;
            }
        }

        if (op == OP_READ) {
            *data = spi->regs.raw[offset >> 2];
        } else if (op == OP_WRITE) {
            spi->regs.raw[offset >> 2] = *data;
        }
    }

    return 0;
}

static int spi_slave_cycle(struct spi_state *spi)
{
    int push = spi->regs.fmt.data.Tx.Tx0 & 1;
    uint32_t SS = spi->regs.fmt.SS;
    if (spi->state != SPI_EMU_RESET && !((SS & (SS - 1)) == 0))
        breakpoint("SPI slave select is not one-hot");

    for (int inst = 0; inst < NINST; inst++) {
        struct spi_ops *ops = &spi->impls[inst];
        void *cookie = spi->impl_cookies[inst];

        // a device not having a clock() cb is invalid or empty
        if (!ops->clock)
            continue; // clock() is the only required callback

        // check if we are selected
        if (SS & (1 << inst)) {
            int pull = -1;

            switch (spi->state) {
                case SPI_EMU_RESET:
                    break;
                case SPI_EMU_STARTED:
                    if (ops->select)
                        ops->select(cookie, 1);
                    spi->state = SPI_EMU_SELECTED;
                    break;
                case SPI_EMU_SELECTED:
                    if (spi->cyc > SPI_INIT_CYCLES)
                        spi->state = SPI_EMU_BUSY;
                    break;
                case SPI_EMU_BUSY: {
                    ops->clock(cookie, 1, push, &pull);
                    int width = spi->regs.fmt.ctrl.u.bits.CHAR_LEN;
                    assert(("SPI generated bit is 0 or 1", (pull == 0 || pull == 1)));
                    // Unconditionally shift all 128 bits for simplicity
                    for (int i = 0; i < 4; i++) {
                        uint32_t *o = &spi->regs.raw[i];
                        *o >>= 1;                       // shift down this word
                        *o  &= ~(1 << 31);              // mask off top bit
                        if (i < 3)
                            *o |= (*(o + 1) & 1) << 31; // get top bit from next word
                    }

                    // Insert pulled bit at appropriate place
                    uint32_t *i = &spi->regs.raw[(width - 1) / 32];
                    *i = *i & ~(   1 << ((width - 1) % 32));
                    *i = *i |  (pull << ((width - 1) % 32));

                    if (!--spi->remaining)
                        spi->state = SPI_EMU_DONE;
                    break;
                }
                case SPI_EMU_DONE:
                    if (ops->select)
                        ops->select(cookie, 0);
                    spi->state = SPI_EMU_RESET;
                    break;
            }
        } else {
            switch (spi->state) {
                case SPI_EMU_SELECTED: {
                    int pull = -1;
                    ops->clock(cookie, 0, push, &pull);
                    if (pull != -1)
                        breakpoint("SPI slave generated traffic when not selected");
                    break;
                }
                default:
                    break;
            }
        }
    }

    return 0;
}

static int spi_emu_cycle(struct sim_state *s, void *cookie)
{
    struct spi_state *spi = cookie;
    if (spi->dividend++ >= (unsigned)((spi->regs.fmt.DIVIDER + 1) * 2 - 1)) {
        spi_slave_cycle(spi);

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

