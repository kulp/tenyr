VPI_CFLAGS := $(shell iverilog-vpi --cflags)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I../../src

TAS = ../../tas

ifdef DEBUG
CFLAGS += -O0 -g
CFLAGS += -DDEBUG=$(DEBUG)
endif

all: tenyr glue.vpi

.SUFFIXES: .v .c

vpath %.c ../vpi
vpath %.v ../verilog ../verilog/sim .

VFLAGS += -I../verilog
VFLAGS += -DICARUS -DSIM

tenyr: VFLAGS += -m glue
tenyr: simserial.v icarus_top.v simmem.v seg7.v seglookup.v hex2ascii.v jcounter.v | glue.vpi

seg7test: seg7.v seglookup.v hex2ascii.v seg7_top.v

glue.vpi: callbacks.o serial.o load.o ../../sim.o ../../asm.o ../../obj.o ../../common.o

vpath %.tas ../verilog

%.to: %.tas
	$(TAS) -o$@ $<

%: %.v
	iverilog $(VFLAGS) -Wall -o $@ $^

%.vpi: %.o
	iverilog-vpi -o $@ $^

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

