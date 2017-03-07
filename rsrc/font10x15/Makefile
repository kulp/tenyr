PNGS = $(wildcard ??.png ???.png)
XPMS = $(PNGS:%.png=%.xpm)
RAWS = $(XPMS:%=%.raw)

all: font.png

font.png: $(PNGS)
	montage $^ -tile $(words $^)x1 -geometry +0+0 $@

raw: $(RAWS)

%.xpm: %.png
	convert $< $@

%.xpm.raw: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '.x' > $@ || rm $@

clean:
	$(RM) *.xpm *.xpm.raw
