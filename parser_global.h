#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

struct instruction_list {
    struct instruction *insn;
    struct instruction_list *next;
};

struct instruction_list *tenor_get_parser_result(void);

#endif

