TOP ?= ../..
PERIODS ?= 32

VPI_CFLAGS := $(shell iverilog-vpi --cflags | sed s/-no-cpp-precomp//)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I../../src

ECHO := $(shell which echo)

ifeq ($V,1)
SILENCE =
MAKESTEP = true
else
SILENCE = @
MAKESTEP := $(if $(findstring s,$(MAKEFLAGS)),true,$(ECHO))
endif

TAS = $(TOP)/tas
TLD = $(TOP)/tld

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
vpath %.memh ../verilog

VFLAGS += -I../verilog
VFLAGS += -DSIM

GEN_V += simserial.v
SIM_GET_simserial = tenyr_getchar
SIM_PUT_simserial = tenyr_putchar

tenyr: VFLAGS += -m glue
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       ramwrap.v                                    \
       eib.v                                        \
       | glue.vpi trampoline.memh

seg7test: seg7.v seglookup.v hex2ascii.v seg7_top.v

glue.vpi: callbacks.o serial.o load.o sim.o asm.o obj.o common.o

# don't complain about unused values that we might use in asserts
asm.o sim.o: CFLAGS += -Wno-unused-value
# it's all right for callbacks not to use all their parameters
asm.o: CFLAGS += -Wno-unused-parameter

%.memh:
	$(MAKE) -C ../verilog $@

vpath %.tas ../verilog
vpath %.texe ../../ex

LOGFILE = Top.vcd
run_%: PLUSARGS += +PERIODS=$(PERIODS) \
                   +LOGFILE=$(LOGFILE) \
                   +VECTORS=$(VECTORS) \
                   $(if $(INTERRUPTS),+INTERRUPT_TIMES=$(INTERRUPTS).times \
                                      +INTERRUPT_SIGNALS=$(INTERRUPTS).signals) \
                   #

run_%: VECTORS = ../../ex/$*.vectors.memh
run_%: %.texe tenyr
	vvp -M. tenyr +LOAD=$< $(PLUSARGS)

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

##############################################################################

OUTPUT_OPTION ?= -o $@

$(GEN_V): VFLAGS += -DPUT='$$$(SIM_PUT_$*)' -DGET='$$$(SIM_GET_$*)' -DSTEM='$*'
$(GEN_V): VFLAGS += -E
$(GEN_V): %.v: ../verilog/sim/simwrap.v
	@$(MAKESTEP) "[ IVERILOG -E ] $@"
	$(SILENT)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

COMPILE.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
%.o: %.c
	@$(MAKESTEP) "[ CC ] $<"
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<

%.to: %.tas
	@$(MAKESTEP) "[ TAS ] $<"
	$(SILENT)$(TAS) -o$@ $<

%.texe: %.to
	@$(MAKESTEP) "[ TLD ] $<"
	$(SILENT)$(TLD) -o$@ $<

%: %.v
	@$(MAKESTEP) "[ IVERILOG ] $@"
	$(SILENT)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

%.vpi: %.o
	@$(MAKESTEP) "[ VPI ] $@"
	$(SILENT)iverilog-vpi $(OUTPUT_OPTION) $^

