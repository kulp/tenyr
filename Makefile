CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -g
CFLAGS += -Wall -Wextra

all: tas tsim

tsim tas: ops.h

tas: parser.o lexer.o

lexer.h lexer.c: lexer.l
	flex --header-file=lexer.h -o lexer.c $<

parser.c: parser.y
	yacc -d -o $@ $<

clean:
	$(RM) tas tsim *.o

