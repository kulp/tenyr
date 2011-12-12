#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"
#include "parser.h"
#include "lexer.h"

struct parse_data {
    yyscan_t scanner;
    struct instruction_list *top;
    struct label_list {
        struct label *label;
        struct label_list *next;
    } *labels;
    uint32_t reladdr;
};

int yyparse(struct parse_data *);
void switch_to_stream(FILE *f, yyscan_t yyscanner);

#endif

