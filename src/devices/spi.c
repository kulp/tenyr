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

// we use some plugin definitions but are not a plugin ourselves (this is
// hacky XXX)
#define TENYR_PLUGIN 0
#include "plugin.h"

#include "common.h"
#include "device.h"
#include "spi.h"

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

    // TODO support remaining CTRL bit behaviours (specifically IE)
    spi->regs.fmt.ctrl.u.CTRL = 0x00000000;

    spi->regs.fmt.DIVIDER     = 0x0000ffff;

    spi->regs.fmt.SS          = 0x00000000;
}

static int spi_emu_init(struct sim_state *s, void *cookie, ...)
{
    struct spi_state *spi = *(void**)cookie = malloc(sizeof *spi);

    spi_reset_defaults(spi);

    memset(spi->impls, 0, sizeof spi->impls);

    const char *implname = NULL;
    if (param_get(s, "spi.impl", &implname)) {
        int inst = 0; // TODO support more than one instance
        // If implname contains a slash, treat it as a path ; otherwise, stem
        char buf[256];
        const char *implpath = NULL;
        const char *implstem = NULL;
        param_get(s, "spi.implstem", &implstem); // may not be set ; that's OK
        if (strchr(implname, PATH_SEPARATOR_CHAR)) {
            implpath = implname;
        } else {
            snprintf(buf, sizeof buf, "lib%s"DYLIB_SUFFIX, implname);
            buf[sizeof buf - 1] = 0;
            implpath = buf;
            if (!implstem)
                implstem = implname;
        }

        // TODO consider using RTLD_NODELETE here
        // (seems to break on Mac OS X)
        // currently we leak library handles
        void *libhandle = dlopen(implpath, RTLD_NOW | RTLD_LOCAL);
        if (!libhandle) {
            debug(1, "Could not load %s, trying default library search", implpath);
            libhandle = RTLD_DEFAULT;
        }

#define GET_CB(Stem)                                                \
        do {                                                        \
            char buf[64];                                           \
            snprintf(buf, sizeof buf, "%s_spi_"#Stem, implstem);    \
            void *ptr = dlsym(libhandle, buf);                      \
            spi->impls[inst].Stem = ALIASING_CAST(spi_##Stem,ptr);  \
        } while (0)                                                 \
        //

        tenyr_plugin_host_init(libhandle);

        GET_CB(clock);
        if (!spi->impls[inst].clock) {
            const char *err = dlerror();
            if (err)
                fatal(0, "Failed to locate SPI clock cb for '%s' ; %s", implstem, err);
            else
                fatal(0, "SPI clock cb for '%s' is NULL ? : %s", implstem);
        }

        GET_CB(init);
        GET_CB(select);
        GET_CB(fini);

        if (spi->impls[inst].init)
            if (spi->impls[inst].init(&spi->impl_cookies[inst]))
                debug(1, "SPI attached instance %d returned nonzero from init()", inst);

        // if RTLD_NODELETE worked and were standard, we would dlclose() here
    }

    return 0;
}

static int spi_emu_fini(struct sim_state *s, void *cookie)
{
    struct spi_state *spi = cookie;

    for (int inst = 0; inst < NINST; inst++) {
        if (spi->impls[inst].fini)
            if (spi->impls[inst].fini(&spi->impl_cookies[inst]))
                debug(1, "SPI attached instance %d returned nonzero from fini()", inst);
    }

    free(spi);

    return 0;
}

static int spi_op(struct sim_state *s, void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct spi_state *spi = cookie;
    uint32_t offset = addr - SPI_BASE;
    int regnum = offset >> 2;

    assert(("Address within address space", !(addr & ~PTR_MASK)));
    assert(("Lower bits of offset are cleared", !(offset & 0x3)));

    // "When a transfer is in progress, writing to any register of the SPI
    // Master core has no effect."
    if (spi->state == SPI_EMU_RESET) {
        if (op == OP_READ) {
            *data = spi->regs.raw[regnum];
        } else if (op == OP_WRITE) {
            if (offset == 0x10) { // CTRL register
                uint32_t go_mask = 1 << 8;
                uint32_t new_go_bit =  go_mask & *data;
                uint32_t old_go_bit =  go_mask & spi->regs.raw[regnum];
                uint32_t new_others = ~go_mask & *data;
                uint32_t old_others = ~go_mask & spi->regs.raw[regnum];

                if (!((new_go_bit == old_go_bit) || (new_others == old_others)))
                    breakpoint("GO_BSY bit changed at the same time as other control bits");

                if (new_go_bit) {
                    spi->state = SPI_EMU_STARTED;
                    spi->remaining = spi->regs.fmt.ctrl.u.bits.CHAR_LEN;
                    if (spi->remaining == 0)
                        spi->remaining = 128;
                }
            } else if (offset == 0x18) { // SS register
                if (!spi->regs.fmt.ctrl.u.bits.ASS) {
                    uint32_t old_ss = spi->regs.fmt.SS;
                    for (size_t inst = 0; inst < NINST; inst++) {
                        int new_ss = *data;
                        int changed = (new_ss ^ old_ss) & (1 << inst);
                        int bit = !!(new_ss & (1 << inst));
                        if (changed)
                            spi->impls[inst].select(spi->impl_cookies[inst], bit);
                    }
                }
            }

            spi->regs.raw[regnum] = *data;
        }
    }

    return 0;
}

static int do_one_shift(struct spi_state *spi, int width, int in)
{
    // Unconditionally shift all 128 bits for simplicity
    if (spi->regs.fmt.ctrl.u.bits.LSB) {
        for (int i = 0; i < 4; i++) {
            uint32_t *o = &spi->regs.raw[i];
            *o >>= 1;                       // shift down this word
            *o  &= ~(1 << 31);              // mask off top bit
            if (i < 3)
                *o |= (*(o + 1) & 1) << 31; // make bottom bit from next word our top
        }

        // Insert pulled bit at MSB
        uint32_t *i = &spi->regs.raw[(width - 1) / 32];
        *i = *i & ~( 1 << ((width - 1) % 32));
        *i = *i |  (in << ((width - 1) % 32));
    } else {
        for (int i = 3; i >= 0; i--) {
            uint32_t *o = &spi->regs.raw[i];
            *o <<= 1;                       // shift up this word
            *o  &= ~1;                      // mask off bottom bit
            if (i > 0)
                *o |= (*(o - 1) & (1 << 31)) >> 31; // make top bit from next word our bottom
        }

        // Insert pulled bit at LSB
        uint32_t *i = &spi->regs.raw[0];
        *i = *i & ~1;
        *i = *i | in;
    }

    return 0;
}

static int spi_slave_cycle(struct spi_state *spi)
{
    int push = -1;
    int width = spi->regs.fmt.ctrl.u.bits.CHAR_LEN;

    uint32_t SS = spi->regs.fmt.SS;
    if (spi->state != SPI_EMU_RESET) {
        if (!((SS & (SS - 1)) == 0))
            breakpoint("SPI slave select is not one-hot");

        if (spi->regs.fmt.ctrl.u.bits.LSB) {
            push = spi->regs.fmt.data.Tx.Tx0 & 1;
        } else {
            int inword = (width - 1) % 32;
            push = (spi->regs.raw[(width - 1) / 32] & (1 << inword)) >> inword;
        }

        assert(("SPI pushed bit is 0 or 1", (push == 0 || push == 1)));
    }

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
                    if (ops->select && spi->regs.fmt.ctrl.u.bits.ASS)
                        if (ops->select(cookie, 1))
                            debug(1, "SPI attached instance %d returned nonzero from select()", inst);
                    spi->state = SPI_EMU_SELECTED;
                    break;
                case SPI_EMU_SELECTED:
                    if (spi->cyc > SPI_INIT_CYCLES)
                        spi->state = SPI_EMU_BUSY;
                    break;
                case SPI_EMU_BUSY: {
                    if (ops->clock(cookie, 1, push, &pull))
                        debug(1, "SPI attached instance %d returned nonzero from clock()", inst);
                    assert(("SPI generated bit is 0 or 1", (pull == 0 || pull == 1)));
                    do_one_shift(spi, width, pull);

                    if (!--spi->remaining)
                        spi->state = SPI_EMU_DONE;
                    break;
                }
                case SPI_EMU_DONE:
                    if (ops->select && spi->regs.fmt.ctrl.u.bits.ASS)
                        if (ops->select(cookie, 0))
                            debug(1, "SPI attached instance %d returned nonzero from select()", inst);
                    spi->state = SPI_EMU_RESET;
                    break;
            }
        } else {
            switch (spi->state) {
                case SPI_EMU_SELECTED: {
                    int pull = -1;
                    if (ops->clock(cookie, 0, push, &pull))
                        debug(1, "SPI attached but disabled instance %d returned nonzero from clock()", inst);
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

