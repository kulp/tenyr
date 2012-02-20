#include <assert.h>
#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"
#include "ram.h"

// Allocate space by roughly a page-size (although since there is overhead the
// fact that it is nearly a page size is basically useless since it does not
// fit evenly into pages). Consider allocating header elements separately from
// storage elements and packing nicely into pages.
// TODO make sensitive to run-time page size ?
#define PAGESIZE    4096
#define PAGEWORDS   (PAGESIZE / sizeof(uint32_t))
#define WORDMASK    ((1 << 10) - 1)

struct sparseram_state {
    void *mem;
    void *userdata; ///< transient userdata, used for twalk() support
};

struct element {
    int32_t base : 24;
    struct sparseram_state *state;  ///< twalk() support
    uint32_t space[];
};

static int tree_compare(const void *_a, const void *_b)
{
    const struct element *a = _a;
    const struct element *b = _b;
    return b->base - a->base;
}

static int sparseram_init(struct state *s, void *cookie, ...)
{
    struct sparseram_state *sparseram = *(void**)cookie = malloc(sizeof *sparseram);
    sparseram->mem = NULL;

    return 0;
}

TODO_TRAVERSE_(element)

static int sparseram_fini(struct state *s, void *cookie)
{
    struct sparseram_state *sparseram = cookie;
    // tdestroy() is a glibc extension. Here we generate a list of nodes to
    // delete and then delete them one by one.
    struct todo_node *todo = NULL;
    sparseram->userdata = &todo;

    twalk(sparseram->mem, traverse_element);

    list_foreach(todo_node, t, todo) {
        tdelete(t->what, &sparseram->mem, tree_compare);
        free(t->what);
        free(t);
    }

    free(sparseram);

    return 0;
}

static int sparseram_op(struct state *s, void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct sparseram_state *sparseram = cookie;
    assert(("Address within address space", !(addr & ~PTR_MASK)));

    struct element key = (struct element){ addr & ~WORDMASK, NULL };
    struct element **p = tsearch(&key, &sparseram->mem, tree_compare);
    if (*p == &key) {
        // Currently, a page is allocated even on a read. It is not a very
        // useful optimisation, but we could avoid allocating a page until the
        // first write.
        struct element *node = malloc(PAGESIZE + sizeof *node);
        *node = (struct element){ addr & ~WORDMASK, cookie };
        *p = node;
    }

    assert(("Sparse page address is non-NULL", *p != NULL));
    uint32_t *where = &(*p)->space[addr & WORDMASK];

    if (op == OP_WRITE)
        *where = *data;
    else if (op == OP_READ)
        *data = *where;
    else
        return 1;

    return 0;
}

int sparseram_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { RAM_BASE, RAM_END },
        .op = sparseram_op,
        .init = sparseram_init,
        .fini = sparseram_fini,
    };

    return 0;
}

