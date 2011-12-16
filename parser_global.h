#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"
#include "parser.h"

struct parse_data {
    void *scanner;
    struct {
        unsigned savecol;
        char saveline[512]; // TODO document limit
    } lexstate;
    struct instruction_list *top;
    struct relocation_list {
        struct const_expr *ce;
        uint32_t *dest;     ///< destination word to be updated
        int width;          ///< width in bits of the right-justified immediate
        struct relocation_list *next;
    } *relocs;
    struct label_list {
        struct label *label;
        struct label_list *next;
    } *labels;
    uint32_t reladdr;
};

int tenor_parse(struct parse_data *);

#endif

