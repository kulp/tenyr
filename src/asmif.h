#ifndef ASMIF_H_
#define ASMIF_H_

#include <stdio.h>

struct format;

int format_has_output(const struct format *f);
int do_assembly(FILE *in, FILE *out, const struct format *f);
int do_disassembly(FILE *in, FILE *out, const struct format *f, int flags);

#endif

