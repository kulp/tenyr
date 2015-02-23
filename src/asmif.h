#ifndef ASMIF_H_
#define ASMIF_H_

#include <stdio.h>

struct format;

int do_assembly(FILE *in, FILE *out, const struct format *f, void *ud);
int do_disassembly(FILE *in, FILE *out, const struct format *f, void *ud, int flags);

#endif

/* vi: set ts=4 sw=4 et: */
