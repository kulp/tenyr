#define _XOPEN_SOURCE (700) /* for strdup */

#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This file must not be linked into plugins, because of the different way
// fatal_ and debug_ are handled in main programs vs. plugins. This file should
// probably be renamed.

jmp_buf errbuf;

// Strip directory components from filename
static const char *notdir(const char *file)
{
    const char *slash = strrchr(file, PATH_COMPONENT_SEPARATOR_CHAR);
    if (slash)
        file = slash + 1; // if we found a slash, start after it

    return file;
}

static void NORETURN main_fatal_(int code, const char *file, int line,
    const char *func, const char *fmt, ...)
{
    va_list vl;
    va_start(vl,fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fprintf(stderr, " (in %s() at %s:%d)", func, notdir(file), line);

    if (code & PRINT_ERRNO)
        fprintf(stderr, ": %s\n", strerror(errno));
    else
        fwrite("\n", 1, 1, stderr);

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
    fprintf(stderr, " (in %s() at %s:%d)\n", func, notdir(file), line);
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

char *build_path(const char *base, const char *fmt, ...)
{
    char *dir = strdup(base);
    char *solidus = strrchr(dir, PATH_COMPONENT_SEPARATOR_CHAR);
    if (solidus)
        solidus[1] = '\0';
    if (!fmt)
        return dir;

    va_list vl;
    va_start(vl, fmt);

    size_t flen = strlen(fmt) + strlen(dir) + 1;
    char *ff = malloc(flen);
    snprintf(ff, flen, "%s%s", solidus ? dir : "", fmt);
    size_t len = vsnprintf(NULL, 0, ff, vl);
    va_start(vl, fmt); // restart
    char *buf = malloc(len + 1);
    vsnprintf(buf, len + 1, ff, vl);

    free(ff);
    free(dir);
    va_end(vl);

    return buf;
}

// These are the implementations of the `common` functions, main program version.
// TODO these should probably be wrapped up in a structure
void (* NORETURN fatal_)(int code, const char *file, int line, const char *func, const char *fmt, ...) = main_fatal_;
void (*debug_)(int level, const char *file, int line, const char *func, const char *fmt, ...) = main_debug_;

/* vi: set ts=4 sw=4 et: */
