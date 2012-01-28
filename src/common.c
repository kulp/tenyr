#include "common.h"

#include <stdio.h>

jmp_buf errbuf;

void fatal(const char *message, enum errcode code)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    longjmp(errbuf, code);
}

