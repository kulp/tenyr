#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

jmp_buf errbuf;

void fatal_(int code, const char *file, int line, const char *fmt, ...)
{
    va_list vl;
    va_start(vl,fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fprintf(stderr, " (source %s:%d)", file, line);

    if (code & PRINT_ERRNO)
        fprintf(stderr, ": %s\n", strerror(errno));
    else
        fputc('\n', stderr);

    longjmp(errbuf, code);
}

