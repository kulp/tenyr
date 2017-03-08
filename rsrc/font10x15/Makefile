PNGS = $(wildcard ??.png ???.png)
XPMS = $(PNGS:%.png=%.xpm)
RAWS = $(XPMS:%=%.raw)

NUMS = $(shell seq 0 255)
ALLPNGS = $(NUMS:%=%.png)

all: font-invert.png font10x15.memb

count: $(RAWS)
	sort $^ | uniq -c | sort -k2 | wc -l

crush: $(PNGS)
	for p in $^ ; do pngcrush -ow $p ; done

%.gen.xpm: make_xpm.pl
	$(realpath $<) $* > $@

%.png: %.gen.xpm
	convert $< $@

font.png: $(ALLPNGS)
	montage $^ -tile $(words $^)x1 -geometry +0+0 $@

%-invert.png: %.png
	convert -negate $< $@

raw: $(RAWS)

%.xpm: %.png
	convert $< $@

%.memb: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '01' > $@ || rm $@

font10x15.memb: $(NUMS:%=%.memb)
	cat $^ > $@

clean:
	$(RM) *.xpm *.memb
