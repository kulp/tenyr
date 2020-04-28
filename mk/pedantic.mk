PEDANTIC = 1

ifeq ($(PEDANTIC),)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors -Wno-error=unknown-warning-option

PEDANTIC_FLAGS += -Werror=covered-switch-default
PEDANTIC_FLAGS += -Werror=missing-variable-declarations
PEDANTIC_FLAGS += -Werror=switch-enum

endif
