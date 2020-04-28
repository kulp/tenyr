PEDANTIC = 1

ifeq ($(PEDANTIC),)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors -Wno-error=unknown-warning-option

# Unreachable `return` statements after `fatal` are not an error for us.
PEDANTIC_FLAGS += -Wno-unreachable-code-return

PEDANTIC_FLAGS += -Werror=covered-switch-default
PEDANTIC_FLAGS += -Werror=missing-variable-declarations
PEDANTIC_FLAGS += -Werror=switch-enum

endif
