.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib

TARGETS = forty.texe
CLEANFILES += $(TARGETS)

all: $(TARGETS)

forty.texe: main.to level0.to level1.to

%.tas: %.tas.cpp
	cpp $(CPPFLAGS) $< -o $@

%.to: %.tas
	$(TAS) -o$@ $< || (rm $@ ; exit 1)

%.texe: %.to
	$(TLD) -o$@ $^ || (rm $@ ; exit 1)

clean:
	$(RM) *.to $(CLEANFILES)

clobber: clean

