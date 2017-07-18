pairs.%: allpairs.%
	sort $^ | uniq $(>@)

# allpairs.% assumes even number of input lines
allpairs.%: %
	cat $< | (while read a && read b ; do echo "$$a	$$b" ; done) $(>@)

pairlist: $(MEMS:%=pairs.%)
	sort --merge $^ | uniq $(>@)

# flip polarity of first 9 bits, leaving any 10th bit alone (because we assume
# the 10th bit is always zero as a padding column between glyphs)
flip.%: %
	perl -pe 's{([01]{9})([01]|\b)}{ chomp(local $$_ = $$1); y/01/10/; $$_.$$2 }ge' $< $(>@)

# needs GNU head for --quiet
flips: $(MEMS:%=flip.%)
	ghead --quiet --lines=14 $^ $(>@)

# retain only bitstrings that begin with `1`
pre1.%: %
	grep '^1' $< $(>@)

# find bitstrings that are redundant with their flips
dups.%: % flip.pre1.%
	sort $^ | uniq -d $(>@)

# maps something to its negative (with caveats for last column and row being forced to 0)
# order of the dependencies matters to recursion, so manually swap back the ordering
map.%: % flip.%
	paste $(word 2,$^) $< $(>@)

# turn a tab-separated `map` pair into a perl script that does replacements
rewrite.%: map.%
	sed 's#^#s/\\b#; s#	#\\b/#; s#$$#/g;#;' $< $(>@)

# deduplicates lines in % by substituting in the `flip` version based on `map`.
# We would prefer to use `%` and `map.flip.dups.%` as targets, but Make refuses
# to recurse through the `flip.%` rule, even though the `%` is different at the
# different call sites
swap.%: flip.% rewrite.dups.%
	perl -p $(word 2,$^) $< $(>@)

# indexswap.% executes a generated script against % (an index) to replace
# flippable bitstrings with their flips
indexswap.%: rewrite.dups.index.memb %
	perl -p $^ $(>@)

pairindex.memb: indexswap.pairlist
	sort $^ | uniq $(>@)

# encode a .memb file pairwise with reference to pairindex.memb
PAIRINDEX_BITS = $(shell perl -MPOSIX=ceil -e '@a=<>, print ceil(log(@a)/log(2))' pairindex.memb)
paircoded.%.memb: indexswap.allpairs.%.memb pairindex.memb
	cat $< | while read b ; do fgrep -hn "$$b" $(word 2,$^) ; done | cut -d: -f1 | perl -lne 'chomp, print unpack "b$(PAIRINDEX_BITS)", pack "C", $$_-1' $(>@)

# XXX this is not quite right, because we don't have information about
# indexswap (compressed pairindex vs uncompressed pairindex)
# TODO keep track during encoding of when we used a flipped index entry, so
# that we can correctly reconstruct here
pairdecode.%.memb: paircoded.%.memb pairindex.memb
	perl -lne 'chomp, print 1+unpack "C", pack "b$(PAIRINDEX_BITS)", $$_' $< | sed 's/$$/p/' | while read b ; do sed -n $$b $(word 2,$^) ; done | tr '\t' '\n' $(>@)

# find lines that are only in the first file
col1.%: % indexswap.%
	comm -2 -3 $^ $(>@)

# find line numbers of lines that are only in the first file
differ.%: % col1.%
	fgrep -n -f $(word 2,$^) $< | cut -d: -f1 $(>@)

# turn line numbers of differences into 1-bits
bitcol1.%: % differ.%
	(seq 1 $(firstword $(shell wc -l $<)) ; cat $(word 2,$^)) | sort -g | uniq -c | while read a b ; do echo $$(( a == 2 )) ; done $(>@)

clean::
	$(RM) pairindex.memb pairlist indexswap.pairlist
