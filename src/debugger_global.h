#ifndef DEBUGGER_PARSER_GLOBAL_H_
#define DEBUGGER_PARSER_GLOBAL_H_

#include "common.h"

struct debugger_data {
    void *scanner;
    struct {
        unsigned savecol;
        char saveline[LINE_LEN];
    } lexstate;
    struct debug_cmd {
        enum {
            CMD_NULL,
            CMD_PRINT,
            CMD_SET_BREAKPOINT,
            CMD_DELETE_BREAKPOINT,
            CMD_CONTINUE,
            CMD_STEP_INSTRUCTION,
            CMD_QUIT,
        } code;
        long arg;
    } cmd;
};

int tdbg_parse(struct debugger_data *);
int tdbg_prompt(struct debugger_data *dd, FILE *where);

#endif

