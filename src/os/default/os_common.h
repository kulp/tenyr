#ifndef OS_COMMON_H_
#define OS_COMMON_H_

#include <stddef.h>

typedef size_t lfind_size_t;

static inline int os_set_binmode(void *stream)
{
    /* no-op */
    (void)stream;
    return 0;
}

#endif

/* vi: set ts=4 sw=4 et: */
