#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

jmp_buf errbuf;

void fatal_(int code, const char *file, int line, const char *func,
            const char *fmt, ...)
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

// tdestroy() is a glibc extension. Here we generate a list of nodes to delete
// and then delete them one by one.
int tree_destroy(struct todo_node **todo, void **tree, traverse *trav, cmp *comp)
{
    *todo = NULL;

    twalk(*tree, trav);

    list_foreach(todo_node, t, *todo) {
        tdelete(t->what, tree, comp);
        free(t->what);
        free(t);
        t = NULL;
    }

    *todo = NULL;

    return 0;
}

