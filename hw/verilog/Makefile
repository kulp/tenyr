VPI_CFLAGS := $(shell iverilog-vpi --cflags)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)
CFLAGS += -I../../src

ifdef DEBUG
CFLAGS += -O0 -g
CFLAGS += -DDEBUG=$(DEBUG)
endif

all: tenyr glue.vpi

.SUFFIXES: .v

tenyr: VFLAGS += -m glue
tenyr: | glue.vpi

glue.vpi: callbacks.o serial.o load.o ../../sim.o ../../asm.o ../../obj.o ../../common.o

%: %.v
	iverilog $(VFLAGS) -s Top -Wall -o $@ $^

%.vpi: %.o
	iverilog-vpi -o $@ $^

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

