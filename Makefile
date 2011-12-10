CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -g
CFLAGS += -Wall -Wextra

all: tas tsim

tsim tas: ops.h

tas: parser.o lexer.o parser.h

lexer.h lexer.c: lexer.l
	flex --header-file=lexer.h -o lexer.c $<

parser.h parser.c: parser.y lexer.h
	yacc -d -o parser.c $<

clean:
	$(RM) tas tsim *.o

clobber: clean
	$(RM) {parser,lexer}.[ch]
	$(RM) -r *.dSYM

