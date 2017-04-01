# Turn off built-in old-style suffix rules
.SUFFIXES:

PNGS = $(wildcard ?.png ??.png ???.png)
XPMS = $(PNGS:.png=.xpm)
MEMS = $(XPMS:.xpm=.memb)

all: invert.font10x15.png rev.font10x15.memb encoded.font10x15.memb

index.memb: $(MEMS)
	sort $^ | uniq > $@ || rm $@

pairs.%: %
	cat $< | (while read a && read b ; do echo "$$a	$$b" ; done) | sort | uniq > $@ || rm $@

pairlist: $(MEMS:%=pairs.%)
	sort --merge $^ | uniq > $@ || rm $@

# Flip polarity of first 9 bits, forcing last column to 0
flip.%: %
	perl -pe 's{\b([01]{9})}{ chomp(local $$_ = $$1); y/01/10/; $$_ }ge' $< > $@ || rm $@

# needs GNU head for --quiet
flips: $(MEMS:%=flip.%)
	ghead --quiet --lines=14 $^ > $@ || rm $@

# retain only bitstrings that begin with `1`
pre1.%: %
	grep '^1' $< > $@ || rm $@

# find bitstrings that are redundant with their flips
dups.%: % flip.pre1.%
	sort $^ | uniq -d > $@ || rm $@

# maps something to its negative (with caveats for last column and row being forced to 0)
map.%: % flip.%
	paste $^ > $@ || rm $@

# turn a tab-separated `map` pair into a perl script that does replacements
rewrite.%: map.%
	sed 's#^#s/\\b#; s#	#\\b/#; s#$$#/g;#;' $< > $@ || rm $@

# deduplicates lines in % by substituting in the `flip` version based on `map`.
# We would prefer to use `%` and `map.flip.dups.%` as targets, but Make refuses
# to recurse through the `flip.%` rule, even though the `%` is different at the
# different call sites
swap.%: flip.% rewrite.dups.%
	perl -p $(word 2,$^) $< > $@ || rm $@

pairlist.swapped: pairlist rewrite.dups.index.memb
	perl -p $(word 2,$^) $< > $@ || rm $@

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
	rev $< > $@ || rm $@

%.xpm: %.png
	convert $< $@

%.memb: %.xpm
	sed -n '/pixels/,22{/^"/p;}' $< | tr -d '",' | tr ' .' '01' > $@ || rm $@

font10x15.memb: $(MEMS)
	cat $^ > $@ || rm $@

encoded.%.memb: %.memb index.memb | encoder.pl
	perl encoder.pl $< > $@ || rm $@

decoded.%.memb: encoded.%.memb index.memb | decoder.pl
	perl decoder.pl $< > $@ || rm $@

clean:
	$(RM) *.xpm *.memb
