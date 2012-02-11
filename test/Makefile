.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib

all:

%.to: %.tas
	cpp $(CPPFLAGS) $^ | $(TAS) -o$@ -

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to

