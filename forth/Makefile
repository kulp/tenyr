.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib
vpath %.tas.cpp ../lib

TARGETS = forty.texe
CLEANFILES += $(TARGETS)

all: $(TARGETS)

forty.texe: main.to level0.to level1.to stringtable.to strtol.to memcpy.to

%.tas: %.tas.cpp forth_common.th
	cpp $(CPPFLAGS) $< -o $@

%.to: %.tas
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) *.to $(CLEANFILES)

clobber: clean

