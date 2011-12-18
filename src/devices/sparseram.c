#include <assert.h>
#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"

struct element {
    int32_t addr : 24;
    uint32_t value;
};

struct sparseram_state {
    void *mem;
};

static int tree_compare(const void *_a, const void *_b)
{
    const struct element *a = _a;
    const struct element *b = _b;
    return b->addr - a->addr;
}

static int sparseram_init(struct state *s, void *cookie, ...)
{
    struct sparseram_state *sparseram = *(void**)cookie = malloc(sizeof *sparseram);
    sparseram->mem = NULL;

    return 0;
}

static int sparseram_fini(struct state *s, void *cookie)
{
    struct sparseram_state *sparseram = cookie;
    // tdestroy() is a glibc extension. Here we generate a list of nodes to
    // delete and then delete them one by one (albeit using a GCC extension,
    // nested functions, in the process).
    struct node {
        void *what;
        struct node *next;
    } *todo = NULL;

    void traverse_action(const void *node, VISIT order, int level)
    {
        if (order == leaf || order == preorder) {
            struct node *here = malloc(sizeof *here);
            here->what = *(void**)node;
            here->next = todo;
            todo = here;
        }
    }

    twalk(sparseram->mem, traverse_action);

    while (todo) {
        struct node *temp = todo;
        todo = todo->next;
        tdelete(temp->what, &sparseram->mem, tree_compare);
        free(temp->what);
        free(temp);
    }

    free(sparseram);

    return 0;
}

static int sparseram_op(struct state *s, void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct sparseram_state *sparseram = cookie;
    assert(("Address within address space", !(addr & ~PTR_MASK)));

    // TODO elucidate ops (right now 1=Write, 0=Read)
    if (op == 1) {
        struct element *key = malloc(sizeof *key);
        *key = (struct element){ addr, *data };
        struct element **p = tsearch(key, &sparseram->mem, tree_compare);
        assert(("Tree insertion succeeded", p != NULL));
        if (*p != key)
            // already existed in tree elsewhere
            free(key);
        // node might have been in tree already ; update its components
        **p = *key;
    } else if (op == 0) {
        struct element *key = malloc(sizeof *key);
        *key = (struct element){ addr, 0 };
        struct element **p = tsearch(key, &sparseram->mem, tree_compare);
        assert(("Tree lookup succeeded", p != NULL));
        *data = (*p)->value;
    } else
        return 1;

    return 0;
}

int sparseram_add_device(struct device *device)
{
    *device = (struct device){
        .bounds[0] = 0,
        .bounds[1] = (1 << 24) - 1,
        .op = sparseram_op,
        .init = sparseram_init,
        .fini = sparseram_fini,
    };

    return 0;
}

