VPI_CFLAGS := $(shell iverilog-vpi --cflags | sed s/-no-cpp-precomp//)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I../../src

TAS = ../../tas
TLD = ../../tld

ifdef DEBUG
CFLAGS += -O0 -g
CFLAGS += -DDEBUG=$(DEBUG)
endif

all: tenyr glue.vpi

.SUFFIXES: .v .c

vpath %.c ../vpi ../../src
vpath %.v ../verilog ../verilog/sim .

VFLAGS += -I../verilog
VFLAGS += -DICARUS -DSIM

tenyr: VFLAGS += -m glue
tenyr: simserial.v icarus_top.v simmem.v seg7.v seglookup.v hex2ascii.v jcounter.v | glue.vpi

seg7test: seg7.v seglookup.v hex2ascii.v seg7_top.v

glue.vpi: callbacks.o serial.o load.o sim.o asm.o obj.o common.o

# don't complain about unused values that we might use in asserts
asm.o sim.o: CFLAGS += -Wno-unused-value
# it's all right for callbacks not to use all their parameters
asm.o: CFLAGS += -Wno-unused-parameter

vpath %.tas ../verilog

run_%: tenyr %.texe
	vvp -M. $< +LOAD=$*.texe

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

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

%.to: %.tas
ifneq ($(MAKE_VERBOSE),)
	$(TAS) -o$@ $<
else
	@echo "[ TAS ] $<"
	@$(TAS) -o$@ $<
endif

%.texe: %.to
ifneq ($(MAKE_VERBOSE),)
	$(TLD) -o$@ $<
else
	@echo "[ TLD ] $<"
	@$(TLD) -o$@ $<
endif

%: %.v
ifneq ($(MAKE_VERBOSE),)
	iverilog $(VFLAGS) -Wall -o $@ $^
else
	@echo "[ IVERILOG ] $@"
	@iverilog $(VFLAGS) -Wall -o $@ $^
endif

%.vpi: %.o
ifneq ($(MAKE_VERBOSE),)
	iverilog-vpi -o $@ $^
else
	@echo "[ VPI ] $@"
	@iverilog-vpi -o $@ $^
endif

