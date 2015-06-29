#ifndef OS_COMMON_H_
#define OS_COMMON_H_

#include <stddef.h>

/*
 * MinGW lfind() and friends use `unsigned int *` where they should use a
 * `size_t *` according to the man page.
 */
typedef unsigned int lfind_size_t;

// Return nonzero from path_walker to end walking
typedef int path_walker(size_t len, const char *name, void *ud);
void os_walk_path_search_list(path_walker walker, void *ud);

#endif

/* vi: set ts=4 sw=4 et: */
