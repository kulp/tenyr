#ifndef OBJ_H_
#define OBJ_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

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
};

struct obj_v0 {
    struct obj base;

    UWord length;       ///< total length of object in words, minimum 2
    UWord flags;        ///< flags

    UWord rec_count;    ///< count of records, minimum 0
    struct objrec {
        struct objrec *next, *prev;

        UWord addr;     ///< base address
        UWord size;     ///< length of record in words, mininum 0
        UWord *data;    ///< contents of record
    } *records;         ///< not an array, necessarily ; use linked list only

    UWord sym_count;    ///< count of symbols, minimum 0
    struct objsym {
        struct objsym *next, *prev;

        int flags;      ///< unused so far (eventually indicate relocations ?)
        char name[32];
        UWord value;
    } *symbols;
};

int obj_write(struct obj *o, FILE *out);
int obj_read(struct obj *o, size_t *size, FILE *in);
void obj_free(struct obj *o);

#endif

