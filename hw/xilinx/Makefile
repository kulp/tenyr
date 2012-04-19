TAS = ../../tas

%.tas: %.to
	$(TAS) -d $< > $@

%.coe: %.tas
	echo 'memory_initialization_radix = 16;' > $@
	echo 'memory_initialization_vector =' >> $@
	$(TAS) -f text $< | sed 's/^0x//' >> $@
	echo ';' >> $@

