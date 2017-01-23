#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "os_common.h"
#include "device.h"
#include "sim.h"
#include "ram.h"

// Allocate space by roughly a page-size (although since there is overhead the
// fact that it is nearly a page size is basically useless since it does not
// fit evenly into pages). Consider allocating header ram_elements separately from
// storage ram_elements and packing nicely into pages.

struct sparseram_state {
    void *mem;
    long pagesize;
    uint32_t mask;
};

struct ram_element {
    int32_t base;
    uint32_t *space;
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
    sparseram->pagesize = os_getpagesize();
    // `mask` has only the bits set that address *within* a page
    sparseram->mask = sparseram->pagesize / sizeof(uint32_t) - 1;

    return 0;
}

static int sparseram_fini(void *cookie)
{
    struct sparseram_state *sparseram = cookie;
    while (sparseram->mem) {
        struct ram_element **np = sparseram->mem, *p  = *np;
        tdelete(p, &sparseram->mem, tree_compare);
        free(p->space);
        free(p);
    }
    free(sparseram);

    return 0;
}

static int sparseram_op(void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct sparseram_state *sparseram = cookie;

    uint32_t page_base = addr & ~sparseram->mask;
    struct ram_element key = (struct ram_element){ page_base, NULL };
    struct ram_element **p = tsearch(&key, &sparseram->mem, tree_compare);
    if (*p == &key) {
        // Currently, a page is allocated even on a read. It is not a very
        // useful optimisation, but we could avoid allocating a page until the
        // first write.
        struct ram_element *node = malloc(sizeof *node);
        node->base  = page_base;
        node->space = malloc(sparseram->pagesize);
        // set all 32-bit words to 0xffffffff, the trap instruction
        memset(node->space, 0xff, sparseram->pagesize);

        *p = node;
    }

    uint32_t *where = &(*p)->space[addr & sparseram->mask];

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
