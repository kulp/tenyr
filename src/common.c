#include "common.h"

#include <stdarg.h>
#include <stdio.h>

jmp_buf errbuf;

void fatal_(enum errcode code, const char *file, int line, const char *fmt, ...)
{
    va_list vl;
    va_start(vl,fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fprintf(stderr, " (source %s:%d)\n", file, line);
    longjmp(errbuf, code);
}

