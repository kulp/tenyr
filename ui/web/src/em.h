#ifndef EM_H_
#define EM_H_

#include <stddef.h>

void *lfind(const void *key, const void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));
void *lsearch(const void *key, void *base, size_t *nelp, size_t width, int (*compar)(const void *, const void *));

#endif
