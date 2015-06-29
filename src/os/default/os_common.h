#ifndef OS_COMMON_H_
#define OS_COMMON_H_

#include <stddef.h>

typedef size_t lfind_size_t;

char *build_path(const char *base, const char *fmt, ...);
// Return nonzero from path_walker to end walking
typedef int path_walker(size_t len, const char *name, void *ud);
// Returns walker's last result (stops walking when walker returns true), or
// returns 0 if PATH is empty or missing.
int os_walk_path_search_list(path_walker walker, void *ud);

#endif

/* vi: set ts=4 sw=4 et: */
