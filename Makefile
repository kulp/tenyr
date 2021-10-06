# delete all build products built by a rule that exits nonzero
.DELETE_ON_ERROR:

ifneq ($V,1)
.SILENT:
endif

clean:
	cmake --build build --target clean

.DEFAULT_GOAL = all

################################################################################
all:
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS}
	cmake --build build

check: all icarus
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS} -DTESTING=1
	cmake --build build
	export PATH=$(abspath .):$$PATH && cd build && ctest --rerun-failed --output-on-failure

ifneq ($(ICARUS),0)
%.vpi: INCLUDES += src

vpath %.c src
vpath %.c hw/vpi

%.vpi: CFLAGS  += $(shell iverilog-vpi --cflags 2> /dev/null | sed s/-no-cpp-precomp//)
%.vpi: CFLAGS  += -Wno-strict-prototypes
%.vpi: LDFLAGS += $(shell iverilog-vpi --ldflags 2> /dev/null)
%.vpi: LDLIBS  += $(shell iverilog-vpi --ldlibs 2> /dev/null)
%.vpi: %,dy.o
	@echo "[ VPI ] $@"
	$(LINK.c) -o $@ $^ $(LDLIBS)

.PHONY: vpi
vpi: vpidevices.vpi
vpidevices.vpi: callbacks,dy.o vpiserial,dy.o load,dy.o sim,dy.o asm,dy.o obj,dy.o common,dy.o param,dy.o stream,dy.o

icarus: tenyr
tenyr: CFLAGS += -Wall -Wextra -Wshadow -pedantic-errors -std=c99 $(VPI_CFLAGS)
tenyr: CFLAGS += -Isrc

vpath %.v hw/verilog hw/verilog/sim hw/icarus
vpath %.v 3rdparty/wb_intercon/rtl/verilog

tenyr: VFLAGS += -Ihw/verilog -g2005-sv
tenyr: VFLAGS += -DSIM

# We use IVERILOG_VPI_MODULE_PATH rather than -L in order to maintain backward
# compatibility with Icarus v10.
tenyr: export IVERILOG_VPI_MODULE_PATH=$(realpath .)
tenyr: VFLAGS += -m vpidevices
tenyr: VFLAGS += -DSIMMEM=GenedBlockMem -DSIMCLK=tenyr_mainclock
tenyr: VFLAGS += -DSERIAL
tenyr: simtop.v top.v                               \
       simserial.v ram.v simclocks.v                \
       seg7.v hex2segments.v                        \
       gpio.v                                       \
       wb_mux.v                                     \
       #

##############################################################################

OUTPUT_OPTION ?= -o $@

%: %.v
	@echo "[ IVERILOG ] $@"
	iverilog $(VFLAGS) -Wall $(OUTPUT_OPTION) $^

else
vpi: ; # VPI support not enabled
icarus: ; # Icarus not enabled
endif
