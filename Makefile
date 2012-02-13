CC       = gcc
CFLAGS  += -g
LDFLAGS += -g

GCC_CFLAGS += -std=c99
GCC_CFLAGS += -Wall -Wextra $(PEDANTIC)

ifeq ($(CC),gcc)
CFLAGS += $(GCC_CFLAGS)
endif

# Optimised build
ifeq ($(DEBUG),)
CFLAGS  += -DNDEBUG -O3
endif

# 32-bit compilation
#CFLAGS  += -m32
#LDFLAGS += -m32

PEDANTIC = -Werror -pedantic-errors

FLEX  = flex
BISON = bison

CFILES = $(wildcard src/*.c) $(wildcard src/devices/*.c)
GENDIR = src/gen

VPATH += src src/devices $(GENDIR)
INCLUDES += src $(GENDIR)

BUILD_NAME := $(shell git describe --tags --long --always)
CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

DEVICES = ram sparseram debugwrap serial
DEVOBJS = $(DEVICES:%=%.o)

all: tas tsim tld
tas tsim tld: common.o
tas: $(GENDIR)/parser.o $(GENDIR)/lexer.o
tas tsim: asm.o obj.o
tsim: $(DEVOBJS) sim.o
testffi: ffi.o sim.o obj.o
tld: obj.o

tas.o tsim.o tld.o: DEFINES += BUILD_NAME='$(BUILD_NAME)'

lexer.o: parser.h

# don't complain about unused values that we might use in asserts
asm.o tsim.o sim.o ffi.o $(DEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
ffi.o asm.o $(DEVOBJS): CFLAGS += -Wno-unused-parameter

# flex-generated code we can't control warnings of as easily
$(GENDIR)/parser.o $(GENDIR)/lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

lexer.h parser.h: $(GENDIR)
tas.o: $(GENDIR)/parser.h

$(GENDIR)/lexer.h $(GENDIR)/lexer.c: lexer.l
	$(FLEX) --header-file=$(GENDIR)/lexer.h -o $(GENDIR)/lexer.c $<

$(GENDIR)/parser.h $(GENDIR)/parser.c: parser.y lexer.h
	$(BISON) --defines=$(GENDIR)/parser.h -o $(GENDIR)/parser.c $<

$(GENDIR):
	mkdir -p $@

ifndef INHIBIT_DEPS
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CFILES:.c=.d)
endif

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
endif

clean:
	$(RM) tas tsim tld testffi *.o *.d src/*.d src/devices/*.d $(GENDIR)/*.d $(GENDIR)/*.o

clobber: clean
	$(RM) $(GENDIR)/{parser,lexer}.[ch]
	$(RM) -r *.dSYM

