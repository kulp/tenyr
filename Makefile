CC       = gcc
CFLAGS  += -std=gnu99
CFLAGS  += -g
LDFLAGS += -g
CFLAGS  += -Wall -Wextra

CFILES = $(wildcard src/*.c) parser.c

VPATH += src
INCLUDES += src

BUILD_NAME := $(shell git describe --long --always)
DEFINES += BUILD_NAME='$(BUILD_NAME)'
CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

all: tas tsim
tas: parser.o lexer.o
tas tsim: asm.o
tsim: ram.o

# we sometimes pass too many arguments to printf
asm.o: CFLAGS += -Wno-format
# don't complain about unused values that we might use in asserts
ram.o tsim.o: CFLAGS += -Wno-unused-value

# flex-generated code we can't control warnings of as easily
lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused

lexer.h lexer.c: lexer.l
	flex --header-file=lexer.h -o lexer.c $<

parser.h parser.c: parser.y lexer.h
	bison --defines=parser.h -o parser.c $<

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CFILES:.c=.d)
endif

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	$(RM) tas tsim *.o *.d

clobber: clean
	$(RM) {parser,lexer}.[ch]
	$(RM) -r *.dSYM

