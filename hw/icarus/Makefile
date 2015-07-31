TOP := $(abspath ../..)
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

PERIODS_DEFAULT ?= 32

CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I$(TOP)/src

INCLUDES += $(INCLUDE_OS)

clean_FILES = tenyr *.o
clobber_FILES  = *.vcd
clobber_FILES += *.texe
clobber_FILES += $(GEN_V)

all: tenyr

.SUFFIXES: .v .c

vpath %.v ../verilog ../verilog/sim .
vpath %.v $(TOP)/3rdparty/wb_intercon/rtl/verilog
vpath %.memh ../verilog

VFLAGS += -I../verilog -g2005-sv
VFLAGS += -DSIM

GEN_V += simserial.v
SIM_GET_simserial = tenyr_getchar
SIM_PUT_simserial = tenyr_putchar

tenyr: VFLAGS += -m vpidevices
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       wb_mux.v                                     \
       #

# don't complain about unused values that we might use in asserts
asm.o sim.o: CFLAGS += -Wno-unused-value
# it's all right for callbacks not to use all their parameters
asm.o: CFLAGS += -Wno-unused-parameter

%.memh:
	$(MAKE) -C ../verilog TOP=$(TOP) $@

vpath %.tas ../verilog
vpath %.texe $(TOP)/ex

LOGFILE = Top.vcd
run_%: PLUSARGS += +PERIODS=$(firstword $(PERIODS) $(PERIODS_$*) $(PERIODS_DEFAULT)) \
                   +LOGFILE=$(LOGFILE) \
                   $(PLUSARGS_EXTRA) \
                   #

run_%: %.texe tenyr
	$(SILENCE)$(IVERILOG)vvp -n -M$(BUILDDIR) tenyr +LOAD=$< $(PLUSARGS)

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


