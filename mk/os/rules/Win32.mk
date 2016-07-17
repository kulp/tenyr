unpack-%: DEST = $(DL_DIR_$*)/$*.tar.gz
unpack-%:
	mkdir -p $(dir $(DEST))
	curl -o $(DEST) $(URL_$*)
	tar -C $(dir $(DEST)) --strip-components 1 -zxf $(DEST)

download-all-sdl2: unpack-SDL2 unpack-SDL2_image

