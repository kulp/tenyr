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
vpath %.memh ../verilog

VFLAGS += -I../verilog -g2005-sv
VFLAGS += -DSIM

# We use IVERILOG_VPI_MODULE_PATH rather than -L in order to maintain backward
# compatibility with Icarus v10.
tenyr: export IVERILOG_VPI_MODULE_PATH=$(BUILDDIR)
tenyr: VFLAGS += -m vpidevices
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       gpio.v                                       \
       wb_mux.v                                     \
       #

# it's all right for callbacks not to use all their parameters
asm.o: CFLAGS += -Wno-unused-parameter

%.memh:
	$(MAKE) -C ../verilog TOP=$(TOP) $@

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
	$(IVERILOG)vvp -N -M$(BUILDDIR) tenyr +LOAD=$< $(PLUSARGS)

# Test for nonexistent file.
.PHONY: nonexistent.texe
run_nonexistent: tenyr
	(! $(IVERILOG)vvp -N tenyr +LOAD=nonexistent.texe)

# Test invalid files.
vpath %.texe $(TOP)/test/misc/obj/
run_bad_magic run_toolarge: run_%: %.texe tenyr
	(! $(IVERILOG)vvp -N tenyr +LOAD=$<)

##############################################################################

OUTPUT_OPTION ?= -o $@

%: %.v
	@$(MAKESTEP) "[ IVERILOG ] $@"
	$(IVERILOG)iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^


