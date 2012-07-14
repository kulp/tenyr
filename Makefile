CC       = $(CROSS_COMPILE)gcc

ifndef NDEBUG
CFLAGS  += -g
LDFLAGS += -g
endif

ifeq ($(WIN32),1)
 EXE_SUFFIX = .exe
 ifeq ($(_32BIT),1)
  CROSS_COMPILE ?= i686-w64-mingw32-
 else
  # lfind() complains about size_t* vs. unsigned int* ; ignore for now
  PEDANTIC =
  CROSS_COMPILE ?= x86_64-w64-mingw32-
 endif
else
 ifeq ($(_32BIT),1)
  CFLAGS  += -m32
  LDFLAGS += -m32
 endif
endif

CFLAGS += -std=c99
CFLAGS += -Wall -Wextra $(PEDANTIC)

# Optimised build
ifeq ($(DEBUG),)
CPPFLAGS += -DNDEBUG
CFLAGS   += -O3
else
CPPFLAGS += -DDEBUG=$(DEBUG)
endif

PEDANTIC ?= -Werror -pedantic-errors

FLEX  = flex
BISON = bison -Werror

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
win32: export _32BIT=1
win32: export WIN32=1
win32: all
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): common.o
tas$(EXE_SUFFIX): $(GENDIR)/parser.o $(GENDIR)/lexer.o
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX): asm.o obj.o
tsim$(EXE_SUFFIX): asm.o obj.o ffi.o \
                   $(GENDIR)/debugger_parser.o \
                   $(GENDIR)/debugger_lexer.o
tsim$(EXE_SUFFIX): $(DEVOBJS) sim.o
tld$(EXE_SUFFIX): obj.o

asm.o: CFLAGS += -Wno-override-init

%$(EXE_SUFFIX): %.o
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

lexer.o: parser.h

# don't complain about unused values that we might use in asserts
tas.o asm.o tsim.o sim.o ffi.o $(DEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
ffi.o asm.o $(DEVOBJS): CFLAGS += -Wno-unused-parameter

# flex-generated code we can't control warnings of as easily
$(GENDIR)/debugger_parser.o $(GENDIR)/debugger_lexer.o \
$(GENDIR)/parser.o $(GENDIR)/lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

$(GENDIR)/lexer.h $(GENDIR)/debugger_lexer.h: $(GENDIR)
tas.o: $(GENDIR)/parser.h
tsim.o: $(GENDIR)/debugger_parser.h

$(GENDIR)/debugger_lexer.h $(GENDIR)/debugger_lexer.c: debugger_lexer.l
	$(FLEX) --header-file=$(GENDIR)/debugger_lexer.h -o $(GENDIR)/debugger_lexer.c $<

$(GENDIR)/debugger_parser.h $(GENDIR)/debugger_parser.c: debugger_parser.y $(GENDIR)/debugger_lexer.h
	$(BISON) --defines=$(GENDIR)/debugger_parser.h -o $(GENDIR)/debugger_parser.c $<

$(GENDIR)/lexer.h $(GENDIR)/lexer.c: lexer.l
	$(FLEX) --header-file=$(GENDIR)/lexer.h -o $(GENDIR)/lexer.c $<

$(GENDIR)/parser.h $(GENDIR)/parser.c: parser.y $(GENDIR)/lexer.h
	$(BISON) --defines=$(GENDIR)/parser.h -o $(GENDIR)/parser.c $<

$(GENDIR):
	mkdir -p $@

.PHONY: install upload
INSTALL_STEM ?= .
INSTALL_DIR  ?= $(INSTALL_STEM)/bin/$(BUILD_NAME)/$(shell $(CC) -dumpmachine)
install: tsim$(EXE_SUFFIX) tas$(EXE_SUFFIX) tld$(EXE_SUFFIX)
	install -d $(INSTALL_DIR)
	install $^ $(INSTALL_DIR)

upload: tsim$(EXE_SUFFIX) tas$(EXE_SUFFIX) tld$(EXE_SUFFIX) | scripts/upload.pl
	$(realpath $|) "$(shell $(CC) -dumpmachine)" "$(shell git describe --tags)" $^

ifndef INHIBIT_DEPS
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CFILES:.c=.d)
endif

# TODO fix .d files ; something is causing many unnecessary rebuilds
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2> /dev/null && \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ && \
	rm -f $@.$$$$ || rm -f $@.$$$$
endif

clean:
	$(RM) tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX) \
	*.o *.d src/*.d src/devices/*.d $(GENDIR)/*.d $(GENDIR)/*.o

clobber: clean
	$(RM) $(GENDIR)/{parser,lexer}.[ch]
	$(RM) -r *.dSYM

