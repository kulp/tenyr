.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

CPP := $(CPP) -

all:

test_imul.texe: imul.to
copy.texe: memcpy.to puts.to

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) < $< > $@

%.to: %.tas
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

