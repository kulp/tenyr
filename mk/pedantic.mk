PEDANTIC = 1

ifeq ($(PEDANTIC),)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors -Wno-error=unknown-warning-option
endif
