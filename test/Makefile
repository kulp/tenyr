.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

all:

copy.texe: memcpy.to puts.to

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) -o $@ $<

%.to: %.tas
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to *.texe

