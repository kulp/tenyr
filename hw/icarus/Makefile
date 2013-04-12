TOP ?= ../..
PERIODS ?= 32

VPI_CFLAGS := $(shell iverilog-vpi --cflags | sed s/-no-cpp-precomp//)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I../../src

TAS = ../../tas
TLD = ../../tld

ifeq ($(WIN32),1)
 OS = Win32
else
 OS = $(shell uname -s)
endif

INCLUDE_OS ?= $(TOP)/src/os/$(OS)

INCLUDES += $(INCLUDE_OS)
CPPFLAGS += $(patsubst %,-D%,$(DEFINES)) \
            $(patsubst %,-I%,$(INCLUDES))

ifdef DEBUG
CFLAGS += -O0 -g
CFLAGS += -DDEBUG=$(DEBUG)
endif

all: tenyr glue.vpi

.SUFFIXES: .v .c

vpath %.c ../vpi ../../src
vpath %.v ../verilog ../verilog/sim .

VFLAGS += -I../verilog
VFLAGS += -DSIM

GEN_V += simserial.v
SIM_GET_simserial = tenyr_getchar
SIM_PUT_simserial = tenyr_putchar

tenyr: VFLAGS += -m glue
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v simmem.v simclocks.v             \
       seg7.v seglookup.v                           \
       hex2ascii.v jcounter.v                       \
       | glue.vpi

seg7test: seg7.v seglookup.v hex2ascii.v seg7_top.v

glue.vpi: callbacks.o serial.o load.o sim.o asm.o obj.o common.o

# don't complain about unused values that we might use in asserts
asm.o sim.o: CFLAGS += -Wno-unused-value
# it's all right for callbacks not to use all their parameters
asm.o: CFLAGS += -Wno-unused-parameter

vpath %.tas ../verilog

LOGFILE = Top.vcd
run_%: PLUSARGS += +LOAD=$*.texe +PERIODS=$(PERIODS) +LOGFILE=$(LOGFILE)
run_%: tenyr %.texe
	vvp -M. $< $(PLUSARGS)

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

##############################################################################

OUTPUT_OPTION ?= -o $@

$(GEN_V): VFLAGS += -DPUT='$$$(SIM_PUT_$*)' -DGET='$$$(SIM_GET_$*)' -DSTEM='$*'
$(GEN_V): VFLAGS += -E
$(GEN_V): %.v: ../verilog/sim/simwrap.v
ifneq ($(MAKE_VERBOSE),)
	iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^
else
	@echo "[ IVERILOG -E ] $@"
	@iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^
endif

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
	iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^
else
	@echo "[ IVERILOG ] $@"
	@iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^
endif

%.vpi: %.o
ifneq ($(MAKE_VERBOSE),)
	iverilog-vpi $(OUTPUT_OPTION) $^
else
	@echo "[ VPI ] $@"
	@iverilog-vpi $(OUTPUT_OPTION) $^
endif

