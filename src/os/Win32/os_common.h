#ifndef OS_COMMON_H_
#define OS_COMMON_H_

/*
 * MinGW lfind() and friends use `unsigned int *` where they should use a
 * `size_t *` according to the man page.
 */
typedef unsigned int lfind_size_t;

#endif

/* vi: set ts=4 sw=4 et: */
