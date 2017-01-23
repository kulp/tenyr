#include <assert.h>
#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "ram.h"

// Allocate space by roughly a page-size (although since there is overhead the
// fact that it is nearly a page size is basically useless since it does not
// fit evenly into pages). Consider allocating header ram_elements separately from
// storage ram_elements and packing nicely into pages.
// TODO make sensitive to run-time page size ?
#define PAGESIZE    4096
#define PAGEWORDS   (PAGESIZE / sizeof(uint32_t))
#define WORDMASK    ((1 << 10) - 1)

struct sparseram_state {
    void *mem;
};

struct ram_element {
    int32_t base;
    struct sparseram_state *state;  ///< twalk() support
    uint32_t space[];
};

static int tree_compare(const void *_a, const void *_b)
{
    const struct ram_element *a = _a;
    const struct ram_element *b = _b;
    return b->base - a->base;
}

static int sparseram_init(struct plugin_cookie *pcookie, void *cookie)
{
    struct sparseram_state *sparseram = *(void**)cookie = malloc(sizeof *sparseram);
    sparseram->mem = NULL;

    return 0;
}

static int sparseram_fini(void *cookie)
{
    struct sparseram_state *sparseram = cookie;
    while (sparseram->mem) {
        struct ram_element **np = sparseram->mem, *p  = *np;
        tdelete(p, &sparseram->mem, tree_compare);
        free(p);
    }
    free(sparseram);

    return 0;
}

static int sparseram_op(void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct sparseram_state *sparseram = cookie;

    struct ram_element key = (struct ram_element){ addr & ~WORDMASK, NULL };
    struct ram_element **p = tsearch(&key, &sparseram->mem, tree_compare);
    if (*p == &key) {
        // Currently, a page is allocated even on a read. It is not a very
        // useful optimisation, but we could avoid allocating a page until the
        // first write.
        struct ram_element *node = malloc(PAGESIZE * sizeof *node->space + sizeof *node);
        for (unsigned long i = 0; i < PAGESIZE; i++)
            node->space[i] = 0xffffffff; // "illegal" ; will trap

        *node = (struct ram_element){ addr & ~WORDMASK, cookie };
        *p = node;
    }

    assert(("Sparse page address is non-NULL", *p != NULL));
    uint32_t *where = &(*p)->space[addr & WORDMASK];

    if (op == OP_WRITE)
        *where = *data;
    else if (op == OP_INSN_READ || op == OP_DATA_READ)
        *data = *where;
    else
        return 1;

    return 0;
}

int sparseram_add_device(struct device *device)
{
    *device = (struct device){
        .bounds = { RAM_BASE, RAM_END },
        .ops = {
            .op = sparseram_op,
            .init = sparseram_init,
            .fini = sparseram_fini,
        },
    };

    return 0;
}

/* vi: set ts=4 sw=4 et: */
