#ifndef DEBUGGER_PARSER_GLOBAL_H_
#define DEBUGGER_PARSER_GLOBAL_H_

#include "common.h"

struct debugger_data {
    void *scanner;
    struct {
        unsigned savecol;
        char saveline[LINE_LEN];
    } lexstate;
    int done;
};

int tdbg_parse(struct debugger_data *);
int tdbg_prompt(struct debugger_data *dd, FILE *where);

#endif

