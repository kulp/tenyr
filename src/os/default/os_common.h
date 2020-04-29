#ifndef OS_COMMON_H_
#define OS_COMMON_H_

#include <stddef.h>
#include <stdio.h>

typedef size_t lfind_size_t;

char *os_find_self(const char *);
FILE *os_fopen(const char *, const char *);
int os_get_tsimrc_path(char buf[], size_t sz);
long os_getpagesize(void);
int os_preamble(void);
int os_set_buffering(FILE *stream, int mode);
int os_set_non_blocking(FILE *stream);

#endif

/* vi: set ts=4 sw=4 et: */
