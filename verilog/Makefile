all: tenyr

.SUFFIXES:
.SUFFIXES: .v

tenyr: tb.v

%: %.v
	iverilog -Wall -o $@ $^

clean:
	$(RM) tenyr

clobber: clean
	$(RM) *.vcd
