#ifndef OS_BASE_H_
#define OS_BASE_H_

// Return nonzero from path_walker to end walking
typedef int path_walker(size_t len, const char *name, void *ud);
// Returns walker's last result (stops walking when walker returns true), or
// returns 0 if PATH is empty or missing.
int os_walk_path_search_list(path_walker walker, void *ud);

#endif

/* vi: set ts=4 sw=4 et: */
