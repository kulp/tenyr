TOP := $(abspath ../..)
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

PERIODS ?= 32

VPI_CFLAGS := $(shell $(IVERILOG)iverilog-vpi --cflags | sed s/-no-cpp-precomp//)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I$(TOP)/src

INCLUDES += $(TOP)/src/os/$(OS)

clean_FILES = tenyr glue.vpi *.o
clobber_FILES = *.vcd

all: tenyr glue.vpi

.SUFFIXES: .v .c

vpath %.c ../vpi $(TOP)/src
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
	$(MAKE) -C ../verilog TOP=$(TOP) $@

vpath %.tas ../verilog
vpath %.texe $(TOP)/ex

LOGFILE = Top.vcd
run_%: PLUSARGS += +PERIODS=$(PERIODS) \
                   +LOGFILE=$(LOGFILE) \
                   +VECTORS=$(VECTORS) \
                   $(if $(INTERRUPTS),+INTERRUPT_TIMES=$(INTERRUPTS).times \
                                      +INTERRUPT_SIGNALS=$(INTERRUPTS).signals) \
                   #

run_%: VECTORS = $(TOP)/ex/$*.vectors.memh
run_%: %.texe tenyr
	$(IVERILOG)vvp -M. tenyr +LOAD=$< $(PLUSARGS)

##############################################################################

OUTPUT_OPTION ?= -o $@

$(GEN_V): VFLAGS += -DPUT='$$$(SIM_PUT_$*)' -DGET='$$$(SIM_GET_$*)' -DSTEM='$*'
$(GEN_V): VFLAGS += -E
$(GEN_V): %.v: ../verilog/sim/simwrap.v
	@$(MAKESTEP) "[ IVERILOG -E ] $@"
	$(SILENCE)$(IVERILOG)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

%: %.v
	@$(MAKESTEP) "[ IVERILOG ] $@"
	$(SILENCE)$(IVERILOG)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

%.vpi: %.o
	@$(MAKESTEP) "[ VPI ] $@"
	$(SILENCE)$(IVERILOG)iverilog-vpi $(OUTPUT_OPTION) $^

