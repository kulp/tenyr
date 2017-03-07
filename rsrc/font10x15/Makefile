PNGS = $(wildcard *.png)
XPMS = $(PNGS:%.png=%.xpm)
RAWS = $(XPMS:%=%.raw)

raw: $(RAWS)

%.xpm: %.png
	convert $< $@

%.xpm.raw: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '.x' > $@ || rm $@

clean:
	$(RM) *.xpm *.xpm.raw
