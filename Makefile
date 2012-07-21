CC = $(CROSS_COMPILE)gcc

-include Makefile.$(shell uname -s)

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

DEVICES = ram sparseram debugwrap serial spi
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

# don't complain about unused values that we might use in asserts
tas.o asm.o tsim.o sim.o ffi.o $(DEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
ffi.o asm.o $(DEVOBJS): CFLAGS += -Wno-unused-parameter

# flex-generated code we can't control warnings of as easily
$(GENDIR)/debugger_parser.o $(GENDIR)/debugger_lexer.o \
$(GENDIR)/parser.o $(GENDIR)/lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

$(GENDIR)/lexer.o tas.o: $(GENDIR)/parser.h
tsim.o: $(GENDIR)/debugger_parser.h

$(GENDIR)/debugger_lexer.o: $(GENDIR)/debugger_parser.h
$(GENDIR)/debugger_lexer.h $(GENDIR)/debugger_lexer.c: debugger_lexer.l | $(GENDIR)
$(GENDIR)/debugger_parser.h $(GENDIR)/debugger_parser.c: debugger_parser.y $(GENDIR)/debugger_lexer.h | $(GENDIR)
$(GENDIR)/lexer.h $(GENDIR)/lexer.c: lexer.l | $(GENDIR)
$(GENDIR)/parser.h $(GENDIR)/parser.c: parser.y $(GENDIR)/lexer.h | $(GENDIR)

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
	$(RM) $(GENDIR)/debugger_parser.[ch] $(GENDIR)/debugger_lexer.[ch] $(GENDIR)/parser.[ch] $(GENDIR)/lexer.[ch]
	-rmdir $(GENDIR)
	$(RM) -r *.dSYM

##############################################################################

OUTPUT_OPTION ?= -o $@

COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
ifneq ($(MAKE_VERBOSE),)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
else
	@echo "[ CC ] $<"
	@$(COMPILE.c) $(OUTPUT_OPTION) $<
endif

LINK.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)

%$(EXE_SUFFIX): %.o
ifneq ($(MAKE_VERBOSE),)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
else
	@echo "[ LD ] $@"
	@$(LINK.c) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endif

$(GENDIR)/%.h $(GENDIR)/%.c: %.l
ifneq ($(MAKE_VERBOSE),)
	$(FLEX) --header-file=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
else
	@echo "[ FLEX ] $<"
	@$(FLEX) --header-file=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
endif

$(GENDIR)/%.h $(GENDIR)/%.c: %.y
ifneq ($(MAKE_VERBOSE),)
	$(BISON) --defines=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
else
	@echo "[ BISON ] $<"
	@$(BISON) --defines=$(GENDIR)/$*.h -o $(GENDIR)/$*.c $<
endif

