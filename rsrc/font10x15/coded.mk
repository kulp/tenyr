SHELL = /usr/bin/env bash

# Turn off built-in old-style suffix rules
.SUFFIXES:

PNGS = $(wildcard ?.png ??.png ???.png)
XPMS = $(PNGS:.png=.xpm)
MEMS = $(XPMS:.xpm=.memb)

>@ = > $@ || (rm $@; false)

TARGETS = encoded.font10x15.memb
all: $(TARGETS)

index.memb: $(MEMS)
	sort $^ | uniq $(>@)

encoded.%.memb: %.memb index.memb | encoder.pl
	perl encoder.pl $< $(>@)

decoded.%.memb: encoded.%.memb index.memb | decoder.pl
	perl decoder.pl $< $(>@)

clean:
	$(RM) index.memb $(TARGETS)
