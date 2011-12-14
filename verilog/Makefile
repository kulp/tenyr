all: tenor

.SUFFIXES:
.SUFFIXES: .v

tenor: tb.v

%: %.v
	iverilog -Wall -o $@ $^

clean:
	$(RM) tenor

clobber: clean
	$(RM) *.vcd
