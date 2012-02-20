CC       = $(CROSS_COMPILE)gcc
CFLAGS  += -g
LDFLAGS += -g

EXE_SUFFIX =

GCC_CFLAGS += -std=c99
GCC_CFLAGS += -Wall -Wextra $(PEDANTIC)

ifneq ($(findstring -gcc, $(CC)),0)
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

all: tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX)
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): common.o
tas$(EXE_SUFFIX): $(GENDIR)/parser.o $(GENDIR)/lexer.o
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX): asm.o obj.o
tsim$(EXE_SUFFIX): asm.o obj.o ffi.o \
                   $(GENDIR)/debugger_parser.o \
                   $(GENDIR)/debugger_lexer.o
tsim$(EXE_SUFFIX): $(DEVOBJS) sim.o
tld$(EXE_SUFFIX): obj.o

asm.o: GCC_CFLAGS += -Wno-override-init

%$(EXE_SUFFIX): %.o
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

lexer.o: parser.h

# don't complain about unused values that we might use in asserts
asm.o tsim.o sim.o ffi.o $(DEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
ffi.o asm.o $(DEVOBJS): CFLAGS += -Wno-unused-parameter

# flex-generated code we can't control warnings of as easily
$(GENDIR)/debugger_parser.o $(GENDIR)/debugger_lexer.o \
$(GENDIR)/parser.o $(GENDIR)/lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

lexer.h parser.h: $(GENDIR)
tas.o: $(GENDIR)/parser.h

$(GENDIR)/debugger_lexer.h $(GENDIR)/debugger_lexer.c: debugger_lexer.l
	$(FLEX) --header-file=$(GENDIR)/debugger_lexer.h -o $(GENDIR)/debugger_lexer.c $<

$(GENDIR)/debugger_parser.h $(GENDIR)/debugger_parser.c: debugger_parser.y debugger_lexer.h
	$(BISON) --defines=$(GENDIR)/debugger_parser.h -o $(GENDIR)/debugger_parser.c $<

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
	$(RM) tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX) \
	*.o *.d src/*.d src/devices/*.d $(GENDIR)/*.d $(GENDIR)/*.o

clobber: clean
	$(RM) $(GENDIR)/{parser,lexer}.[ch]
	$(RM) -r *.dSYM

