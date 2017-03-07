PNGS = $(wildcard ??.png ???.png)
XPMS = $(PNGS:%.png=%.xpm)
RAWS = $(XPMS:%=%.raw)

NUMS = $(shell seq 0 255)
ALLPNGS = $(NUMS:%=%.png)

all: font.png

%.gen.xpm: make_xpm.pl
	$(realpath $<) $* > $@

%.png: %.gen.xpm
	convert $< $@

font.png: $(ALLPNGS)
	montage $^ -tile $(words $^)x1 -geometry +0+0 $@

raw: $(RAWS)

%.xpm: %.png
	convert $< $@

%.xpm.raw: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '.x' > $@ || rm $@

clean:
	$(RM) *.xpm *.xpm.raw
