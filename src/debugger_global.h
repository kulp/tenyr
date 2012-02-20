#ifndef DEBUGGER_PARSER_GLOBAL_H_
#define DEBUGGER_PARSER_GLOBAL_H_

#include "common.h"

struct debug_expr {
    enum expr_type { EXPR_NULL, EXPR_MEM, EXPR_REG } type;
    int32_t val;
};

enum display_type {
    DISP_NULL,

    DISP_DEC,
    DISP_HEX,
    DISP_INST,

    DISP_max
};

struct debugger_data {
    struct state *s;    ///< simulator state to which we belong

    void *scanner;
    void *breakpoints;

    struct {
        unsigned savecol;
        char saveline[LINE_LEN];
    } lexstate;

    struct debug_display {
        struct debug_display *next;

        struct debug_expr expr;
        int fmt;
    } *displays;

    struct debug_cmd {
        enum {
            CMD_NULL,

            CMD_CONTINUE,
            CMD_DELETE_BREAKPOINT,
            CMD_DISPLAY,
            CMD_GET_INFO,
            CMD_PRINT,
            CMD_SET_BREAKPOINT,
            CMD_STEP_INSTRUCTION,
            CMD_QUIT,

            CMD_max
        } code;
        struct {
            struct debug_expr expr;
            int fmt;   ///< print / display format character
            char str[LINE_LEN];
        } arg;
    } cmd;
};

int tdbg_parse(struct debugger_data *);
int tdbg_prompt(struct debugger_data *dd, FILE *where);

#endif

