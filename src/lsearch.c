// Fake lsearch for emscripten support since emscripten doesn't have one
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// em.h defines these prototypes, but this file should not need to depend on
// that header.
void *lfind(const void *key, const void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));
void *lsearch(const void *key, void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));

void *lfind(const void *key, const void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *))
{
    char *where = (void*)base;
    char *end = where + *nelp * width;
    while (where < end && compar(key, where)) {
        where += width;
    }

    if (where < end)
        return where;
    else
        return NULL;
}

void *lsearch(const void *key, void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *))
{
    void *result = lfind(key, base, nelp, width, compar);
    if (result) {
        return result;
    } else {
        char *where = base;
        memcpy(where += ((*nelp)++ * width), key, width);
        return where;
    }
}

