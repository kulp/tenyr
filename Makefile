CC       = gcc
CFLAGS  += -std=c99
CFLAGS  += -g
LDFLAGS += -g
CFLAGS  += -Wall -Wextra $(PEDANTIC)

PEDANTIC = -Werror -pedantic-errors

CFILES = $(wildcard src/*.c) $(wildcard src/devices/*.c) #parser.c lexer.c
GENDIR = src/gen

VPATH += src src/devices $(GENDIR)
INCLUDES += src $(GENDIR)

BUILD_NAME := $(shell git describe --long --always)
DEFINES += BUILD_NAME='$(BUILD_NAME)'
CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

DEVICES = ram sparseram debugwrap
DEVOBJS = $(DEVICES:%=%.o)

all: tas tsim
tas: parser.o lexer.o
tas tsim: asm.o
tsim: $(DEVOBJS)

# sparseram uses some GCC-only constructs (nested functions)
sparseram.o: PEDANTIC=

# we sometimes pass too many arguments to printf
asm.o: CFLAGS += -Wno-format
# don't complain about unused values that we might use in asserts
tsim.o $(DEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
$(DEVOBJS): CFLAGS += -Wno-unused-parameter

# flex-generated code we can't control warnings of as easily
lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused

$(GENDIR)/lexer.h $(GENDIR)/lexer.c: lexer.l
	flex --header-file=$(GENDIR)/lexer.h -o $(GENDIR)/lexer.c $<

$(GENDIR)/parser.h $(GENDIR)/parser.c: parser.y lexer.h
	bison --defines=$(GENDIR)/parser.h -o $(GENDIR)/parser.c $<

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
	$(RM) $(GENDIR)/{parser,lexer}.[ch]
	$(RM) -r *.dSYM

