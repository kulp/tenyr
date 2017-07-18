SHELL = /usr/bin/env bash

# Turn off built-in old-style suffix rules
.SUFFIXES:

PNGS = $(wildcard ?.png ??.png ???.png)
XPMS = $(PNGS:.png=.xpm)
MEMS = $(XPMS:.xpm=.memb)

>@ = > $@ || (rm $@; false)

include coded.mk paircoded.mk

TARGETS = invert.font10x15.png rev.font10x15.memb
all: $(TARGETS)

crush: $(PNGS)
	for p in $^ ; do pngcrush -ow $p ; done

font10x15.png: $(PNGS)
	montage $^ -tile $(words $^)x1 -geometry +0+0 $@

sentence.png: NUMS = $(shell perl -le 'print ord for split //, qq($(PHRASE))')
# We could use .SECONDEXPANSION and then use $+ instead of repeating $(PNGS)
# below, but this way we avoid introducing .SECONDEXPANSION for just one rule
sentence.png: $(PNGS)
	montage $(PNGS) -tile $(words $(PNGS))x1 -geometry +0+0 $@

invert.%.png: %.png
	convert -negate $< $@

# reverse order of bits
rev.%.memb: %.memb
	rev $< $(>@)

%.xpm: %.png
	convert $< $@

%.memb: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '01' $(>@)

font10x15.memb: $(MEMS)
	cat $^ $(>@)

clean::
	$(RM) *.xpm *.memb $(TARGETS)
