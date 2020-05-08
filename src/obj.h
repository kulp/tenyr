#ifndef OBJ_H_
#define OBJ_H_

#include "common.h"
#include "stream.h"

#include <stdio.h>
#include <stdint.h>

// RLC_NEGATE inverts the sign of the relocation adjustment
#define RLC_NEGATE      1
// SYM_BSS marks a symbol as belonging to .bss
#define SYM_BSS         2
// RLC_ABSOLUTE marks a symbol as not being relative to the start of its record
#define RLC_ABSOLUTE    4

typedef int32_t SWord;

struct name {
    char *str;
    SWord len;
};

struct obj {
    union {
        SWord word;     ///< big-endian "TOVn" ; n is a version byte starting at \0
        struct {
            char TOV[3];
            unsigned char version;
        } parsed;
    } magic;

    SWord flags;        ///< flags

    /// each flag is set if the corresponding structure was allocated (and
    /// therefore should be freed) in a single operation
    struct {
        unsigned records:1;
        unsigned symbols:1;
        unsigned relocs:1;
    } bloc;

    SWord rec_count;    ///< count of records, minimum 0
    struct objrec {
        struct objrec *next;

        SWord addr;     ///< base address
        SWord size;     ///< length of record in words, mininum 0
        SWord *data;    ///< contents of record
    } *records;         ///< not an array, necessarily ; use linked list only

    SWord sym_count;    ///< count of symbols, minimum 0
    struct objsym {
        struct objsym *next;

        SWord flags;
        struct name name;
        SWord value;
        SWord size;
    } *symbols;

    SWord rlc_count;    ///< count of relocations
    struct objrlc {
        struct objrlc *next;

        SWord flags;
        struct name name;
        SWord addr;     ///< relative location in the object to update
        SWord width;    ///< width in bits of the right-justified immediate
        SWord shift;    ///< right-shift in bits of the immediate
    } *relocs;
};

int obj_write(struct obj *o, STREAM *out);
int obj_read(struct obj *o, STREAM *in);
void obj_free(struct obj *o);

static inline size_t round_up_to_word(size_t x)
{
    // e.g. (x + 3) & ~3
    return (x + (sizeof(SWord) - 1)) & ~(sizeof(SWord) - 1);
}

#endif

/* vi: set ts=4 sw=4 et: */
