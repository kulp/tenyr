PNGS = $(wildcard 3[2-9].png [4-9]?.png 1[01]?.png 12[0-7].png) # 32 - 127
XPMS = $(PNGS:%.png=%.xpm)
MEMS = $(XPMS:.xpm=.memb)

NUMS = $(shell seq 0 255)
ALLPNGS = $(NUMS:%=%.png)

all: font-invert.png font10x15.memb

index.memb: $(MEMS)
	sort $^ | uniq | sort > $@ || rm $@

count: index.memb
	wc -l $^

crush: $(PNGS)
	for p in $^ ; do pngcrush -ow $p ; done

%.gen.xpm: make_xpm.pl
	$(realpath $<) $* > $@ || rm $@

%.png: %.gen.xpm
	convert $< $@

font.png: $(ALLPNGS)
	montage $^ -tile $(words $^)x1 -geometry +0+0 $@

sentence.png: NUMS = $(shell perl -le 'print ord for split //, qq($(PHRASE))')
# We could use .SECONDEXPANSION and then use $+ instead of repeating $(ALLPNGS)
# below, but this way we avoid introducing .SECONDEXPANSION for just one rule
sentence.png: $(ALLPNGS)
	montage $(ALLPNGS) -tile $(words $(ALLPNGS))x1 -geometry +0+0 $@

%-invert.png: %.png
	convert -negate $< $@

# reverse order of bits
rev-%: %
	rev $< > $@ || rm $@

%.xpm: %.png
	convert $< $@

%.memb: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '01' > $@ || rm $@

font10x15.memb: $(NUMS:%=rev-%.memb)
	cat $^ > $@ || rm $@

encoded.%.memb: %.memb index.memb | encoder.pl
	perl encoder.pl $< > $@ || rm $@

decoded.%.memb: encoded.%.memb index.memb | decoder.pl
	perl decoder.pl $< > $@ || rm $@

clean:
	$(RM) *.xpm *.memb
