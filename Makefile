CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -g
LDFLAGS += -g
CFLAGS += -Wall -Wextra

all: tas tsim
tsim.o tas.o: ops.h
tas.o: parser.h
tas: parser.o lexer.o

lexer.h lexer.c: lexer.l
	flex --header-file=lexer.h -o lexer.c $<

parser.h parser.c: parser.y lexer.h
	yacc -d -o parser.c $<

clean:
	$(RM) tas tsim *.o

clobber: clean
	$(RM) {parser,lexer}.[ch]
	$(RM) -r *.dSYM

