ifneq ($(SDL),0)
SDL2_CONFIG ?= sdl2-config
SDL_VERSION := $(shell $(SDL2_CONFIG) --version 2>/dev/null)
ifneq ($(SDL_VERSION),)
# Use := to ensure the expensive underyling call is not repeated
NO_C11_WARN_OPTS := $(call cc_flag_supp,-Wno-c11-extensions)
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
libtenyrsdl%$(DYLIB_SUFFIX) $(PDEVICES_SDL:%=devices/%.d): \
    CPPFLAGS += $(shell $(SDL2_CONFIG) --cflags) $(NO_C11_WARN_OPTS)
libtenyrsdl%$(DYLIB_SUFFIX): LDLIBS += $(shell $(SDL2_CONFIG) --libs) -lSDL2_image
$(PDEVICES_SDL:%=%,dy.o): PEDANTIC_FLAGS :=
endif
endif

