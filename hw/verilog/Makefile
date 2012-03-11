VPI_CFLAGS := $(shell iverilog-vpi --cflags)
CFLAGS += -Wall -Wextra -pedantic-errors -std=c99 $(VPI_CFLAGS)

CFLAGS += -O0 -g

all: tenyr glue.vpi

.SUFFIXES: .v

tenyr: VFLAGS += -m glue
tenyr: | glue.vpi
tenyr: tb.v

glue.vpi: callbacks.o serial.o load.o

%: %.v
	iverilog $(VFLAGS) -Wall -o $@ $^

%.vpi: %.o
	iverilog-vpi -o $@ $^

clean:
	$(RM) tenyr glue.vpi *.o

clobber: clean
	$(RM) *.vcd

