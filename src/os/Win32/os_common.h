#ifndef OS_COMMON_H_
#define OS_COMMON_H_

#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/time.h>

/*
 * MinGW lfind() and friends use `unsigned int *` where they should use a
 * `size_t *` according to the man page.
 */
typedef unsigned int lfind_size_t;

// timeradd doesn't exist on MinGW
#ifndef timeradd
#define timeradd timeradd

static inline void timeradd(struct timeval *a, struct timeval *b,
                            struct timeval *result)
{
    result->tv_sec  = a->tv_sec  + b->tv_sec;
    result->tv_usec = a->tv_usec + b->tv_usec;
    result->tv_sec += result->tv_usec / 1000000;
    result->tv_usec %= 1000000;
}

#endif

struct param_state;

char *os_find_self(const char *);
FILE *os_fopen(const char *, const char *);
int os_get_tsimrc_path(char buf[], size_t sz);
long os_getpagesize(void);
int os_preamble(void);
int os_set_buffering(FILE *stream, int mode);

#endif

/* vi: set ts=4 sw=4 et: */
