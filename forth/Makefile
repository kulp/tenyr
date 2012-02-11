.SUFFIXES:
.SUFFIXES: .tas .to .texe

TAS = ../tas
TLD = ../tld

CPPFLAGS += -I../lib

vpath %.tas ../lib

TARGETS = forty.texe
CLEANFILES += $(TARGETS)

all: $(TARGETS)

forty.texe: main.to puts.to

%.to: %.tas
	cpp $(CPPFLAGS) $^ | $(TAS) -o$@ -

%.texe: %.to
	$(TLD) -o$@ $^

clean:
	$(RM) $(CLEANFILES)

clobber: clean
	$(RM) *.to

