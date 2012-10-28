// Fake tsearch for emscripten support since emscripten doesn't have one
// Start with a linear search and upgrade it later if needed
// We don't provide a working twalk() yet
#include <stddef.h>
#include <stdlib.h>

void *lfind(const void *key, const void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));
void *lsearch(const void *key, void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));

struct linear_record {
    // need a reference to the comparator function in the linear key itself
    // because there is no properly reentrant way to get it to the lfind()
    // comparator otherwise
    int (*foreign)(const void *key1, const void *key2);
    void *datum;
    int deleted;    // gets set true after node deletion
};

struct tree_state {
    struct linear_record *base;
    size_t used, allocated;
};

static const size_t elt_width = sizeof (struct linear_record);

static int linear_compare(const void *_a, const void *_b)
{
    const struct linear_record *a = _a,
                               *b = _b;

    return b->deleted ? -1 : a->foreign(a->datum, b->datum);
}

void *tfind(const void *key, void *const *rootp, int (*compar)(const void *key1, const void *key2))
{
    struct tree_state *t = *rootp;
    struct linear_record k = {
        .foreign = compar,
        .datum = (void*)key,
    };
    struct linear_record *result = lfind(&k, t->base, &t->used, elt_width, linear_compare);
    // return a pointer to the datum (so that the user can write through it)
    return (result && !result->deleted) ? &result->datum : NULL;
}

void *tsearch(const void *key, void **rootp, int (*compar)(const void *key1, const void *key2))
{
    if (!*rootp) {
        struct tree_state *s = *rootp = malloc(sizeof *s);
        s->used = 0;
        s->allocated = 64;
        s->base = malloc(s->allocated * sizeof *s->base);
    }

    void *result = tfind(key, rootp, compar);
    if (result) {
        return result;
    } else {
        struct tree_state *t = *rootp;
        while (t->allocated - t->used < 1) {
            t->base = realloc(t->base, (t->allocated *= 2) * elt_width);
        }
        struct linear_record *k = malloc(sizeof *k);
        k->foreign = compar;
        k->datum = (void*)key;
        k->deleted = 0;
        struct linear_record *r = lsearch(k, t->base, &t->used, elt_width, linear_compare);
        return &r->datum;
    }
}

void *tdelete(const void *restrict key, void **restrict rootp, int (*compar)(const void *key1, const void *key2))
{
    struct tree_state *t = *rootp;
    struct linear_record k = {
        .foreign = compar,
        .datum = (void*)key,
    };
    struct linear_record *result = lfind(&k, t->base, &t->used, elt_width, linear_compare);
    result->deleted = 1;
    // return always a pointer to our "root" and hope the user doesn't care
    return rootp;
}

void twalk(const void *root, void (*action) (const void *node, int order, int level))
{
    // TODO
    (void)root;
    (void)action;
}

