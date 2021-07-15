TOP := ../..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

PERIODS_DEFAULT ?= 1000000 # One million cycles should be enough for anybody.

CFLAGS += -Wall -Wextra -Wshadow -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I$(TOP)/src

INCLUDES += $(INCLUDE_OS)

clean_FILES = tenyr *.o
clobber_FILES  = *.vcd
clobber_FILES += *.texe

all: tenyr

vpath %.v ../verilog ../verilog/sim .
vpath %.v $(TOP)/3rdparty/wb_intercon/rtl/verilog

VFLAGS += -I../verilog -g2005-sv
VFLAGS += -DSIM

# We use IVERILOG_VPI_MODULE_PATH rather than -L in order to maintain backward
# compatibility with Icarus v10.
tenyr: export IVERILOG_VPI_MODULE_PATH=$(realpath $(BUILDDIR))
tenyr: VFLAGS += -m vpidevices
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       gpio.v                                       \
       wb_mux.v                                     \
       #

vpath %.tas ../verilog
# TODO ensure .texe get rebuilt correctly from here as needed
vpath %.texe $(TOP)/ex $(TOP)/test/run/sdl
vpath %.tas $(TOP)/ex $(TOP)/test/run/sdl

LOGFILE = Top.vcd
run_%: PLUSARGS += +PERIODS=$(PERIODS_DEFAULT) \
                   +LOGFILE=$(LOGFILE) \
                   $(PLUSARGS_EXTRA) \
                   #

run_%: %.texe tenyr
	$(IVERILOG)vvp -N -M$(BUILDDIR) -m vpidevices tenyr +LOAD=$< $(PLUSARGS)

##############################################################################

OUTPUT_OPTION ?= -o $@

%: %.v
	@$(MAKESTEP) "[ IVERILOG ] $@"
	$(IVERILOG)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^


