uniq.%: %
	sort $^ | uniq $(>@)

# allpairs.% assumes even number of input lines
allpairs.%: %
	cat $< | (while read a && read b ; do echo "$$a	$$b" ; done) $(>@)

15to16.%: %
	perl -lpe 'print "0" x 10 unless $$. % 15' $< $(>@)

16to15.%: %
	perl -lne 'print unless $$. % 16 == 15' $< $(>@)

pairlist.memb: $(MEMS:%=uniq.allpairs.15to16.%)
	sort --merge $^ | uniq $(>@)

# encode a .memb file pairwise with reference to uniq.pairlist.memb
PAIRINDEX_BITS = $(shell perl -MPOSIX=ceil -e '@a=<>, print ceil(log(@a)/log(2))' uniq.pairlist.memb)
paircoded.%.memb: allpairs.15to16.%.memb uniq.pairlist.memb
	cat $< | while read b ; do fgrep -hn "$$b" $(word 2,$^) ; done | cut -d: -f1 | perl -lne 'chomp, print unpack "b$(PAIRINDEX_BITS)", pack "C", $$_-1' $(>@)

pairdecode.%.memb: paircoded.%.memb uniq.pairlist.memb
	perl -lne 'chomp, print 1+unpack "C", pack "b$(PAIRINDEX_BITS)", $$_' $< | sed 's/$$/p/' | while read b ; do sed -n $$b $(word 2,$^) ; done | tr '\t' '\n' $(>@)

clean::
	$(RM) uniq.pairlist.memb
