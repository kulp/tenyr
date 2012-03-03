#ifndef OBJ_H_
#define OBJ_H_

#include "common.h"

#include <stdio.h>
#include <stdint.h>

typedef uint32_t UWord;
typedef  int32_t SWord;

struct obj {
    union {
        UWord word;     ///< big-endian "TOVn" ; n is a version byte starting at \0
        struct {
            char TOV[3];
            unsigned char version;
        } parsed;
    } magic;

    UWord flags;        ///< flags

    /// each flag is set if the corresponding structure was allocated (and
    /// therefore should be freed) in a single operation
    struct {
        unsigned records:1;
        unsigned symbols:1;
        unsigned relocs:1;
    } bloc;

    UWord rec_count;    ///< count of records, minimum 0
    struct objrec {
        struct objrec *next;

        UWord addr;     ///< base address
        UWord size;     ///< length of record in words, mininum 0
        UWord *data;    ///< contents of record
    } *records;         ///< not an array, necessarily ; use linked list only

    UWord sym_count;    ///< count of symbols, minimum 0
    struct objsym {
        struct objsym *next;

        UWord flags;    ///< unused so far (eventually indicate relocations ?)
        char name[SYMBOL_LEN];
        UWord value;
    } *symbols;

    UWord rlc_count;    ///< count of relocations
    struct objrlc {
        struct objrlc *next;

        UWord flags;
        char name[SYMBOL_LEN];
        UWord addr;     ///< relative location in the object to update
        UWord width;    ///< width in bits of the right-justified immediate
    } *relocs;
};

int obj_write(struct obj *o, FILE *out);
int obj_read(struct obj *o, FILE *in);
void obj_free(struct obj *o);

#endif

