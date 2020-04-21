#ifndef ASMIF_H_
#define ASMIF_H_

#include "stream.h"

#include <stdio.h>

struct format;

int do_assembly(STREAM *in, STREAM *out, const struct format *f, void *ud);
int do_disassembly(STREAM *in, STREAM *out, const struct format *f, void *ud, int flags);

#endif

/* vi: set ts=4 sw=4 et: */
