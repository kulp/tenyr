#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// This file must not be linked into plugins, because of the different way
// fatal_ and debug_ are handled in main programs vs. plugins. This file should
// probably be renamed.

jmp_buf errbuf;

static void NORETURN main_fatal_(int code, const char *file, int line,
    const char *func, const char *fmt, ...)
{
    va_list vl;
    va_start(vl,fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fprintf(stderr, " (in %s() at %s:%d)", func, file, line);

    if (code & PRINT_ERRNO)
        fprintf(stderr, ": %s\n", strerror(errno));
    else
        fputc('\n', stderr);

    longjmp(errbuf, code);
}

static void main_debug_(int level, const char *file, int line,
    const char *func, const char *fmt, ...)
{
#ifndef DEBUG
#define DEBUG 0
#endif
    if (level > DEBUG)
        return;

    va_list vl;
    va_start(vl,fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fprintf(stderr, " (in %s() at %s:%d)\n", func, file, line);
}

long long numberise(char *str, int base)
{
    char *p = str;
    // there are more efficient ways to strip '_' but this one is pretty clear
    int len = strlen(p);
    while ((p = strchr(p, '_')))
        memmove(p, p + 1, len - (p - str));

    return strtoll(str, NULL, base);
}

// These are the implementations of the `common` functions, main program version.
// TODO these should probably be wrapped up in a structure
void (* NORETURN fatal_)(int code, const char *file, int line, const char *func, const char *fmt, ...) = main_fatal_;
void (*debug_)(int level, const char *file, int line, const char *func, const char *fmt, ...) = main_debug_;

