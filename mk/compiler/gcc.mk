# Exempt ourselves from string-related warnings we have manually vetted
asm.o: CFLAGS += -Wno-stringop-overflow
obj.o: CFLAGS += -Wno-stringop-truncation
