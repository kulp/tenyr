all: tenyr glue.vpi

.SUFFIXES:
.SUFFIXES: .v

tenyr: VFLAGS += -m glue
tenyr: tb.v

%: %.v
	iverilog $(VFLAGS) -Wall -o $@ $^

clean:
	$(RM) tenyr glue.vpi

glue.vpi: callbacks.c

%.vpi: %.c
	iverilog-vpi -o $@ $^

clobber: clean
	$(RM) *.vcd
