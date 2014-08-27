PROJECT = Tenyr
# synchronise $(OUT) with $(PROJECT).qsf's PROJECT_OUTPUT_DIRECTORY
OUT = output_files

all: asm sta

-include $(PROJECT).md
%.md: %.qsf
	perl -nae 'BEGIN{print "$(OUT)/$*.map.rpt:"} /_FILE/ && print "$$F[-1] "' $< > $@.$$$$ && \
	mv $@.$$$$ $@ || \
	rm $@.$$$$

map fit asm sta: %: $(OUT)/$(PROJECT).%.rpt

map_ARGS = --family='Cyclone III'
fit_ARGS = --part=EP3C16F484C6

$(OUT)/$(PROJECT).fit.rpt: $(OUT)/$(PROJECT).map.rpt
$(OUT)/$(PROJECT).asm.rpt: $(OUT)/$(PROJECT).fit.rpt
$(OUT)/$(PROJECT).sta.rpt: $(OUT)/$(PROJECT).fit.rpt

$(OUT)/$(PROJECT).%.rpt:
	quartus_$* $($*_ARGS) $(PROJECT)

clean::
	rm -rf $(OUT)/ db/ incremental_db/ *.htm *.eqn *.pin *.sof *.pof

clobber:: clean
	xargs rm -rf < .gitignore
	git ls-files --others | xargs rm -rf

