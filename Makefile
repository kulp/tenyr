CC       = gcc
CFLAGS  += -std=gnu99
CFLAGS  += -g
LDFLAGS += -g
CFLAGS  += -Wall -Wextra

BUILD_NAME := $(shell git describe --long --always)
DEFINES += BUILD_NAME='$(BUILD_NAME)'
CPPFLAGS += $(patsubst %,-D%,$(DEFINES))

all: tas tsim
tsim.o tas.o: ops.h
tas.o: parser.h
tas: parser.o lexer.o

# flex-generated code we can't control warnings of as easily
lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused

lexer.h lexer.c: lexer.l
	flex --header-file=lexer.h -o lexer.c $<

parser.h parser.c: parser.y lexer.h
	bison -d -o parser.c $<

clean:
	$(RM) tas tsim *.o

clobber: clean
	$(RM) {parser,lexer}.[ch]
	$(RM) -r *.dSYM

