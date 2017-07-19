all: index.memb encoded.font10x15.memb

index.memb: $(MEMS)
	sort $^ | uniq $(>@)

encoded.%.memb: %.memb index.memb | encoder.pl
	perl encoder.pl $< $(>@)

decoded.%.memb: encoded.%.memb index.memb | decoder.pl
	perl decoder.pl $< $(>@)

clean::
	$(RM) index.memb
