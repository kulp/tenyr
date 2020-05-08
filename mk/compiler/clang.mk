# Our use of Clang can still raise warnings for exceptional cases, instead of
# suppressing all instances of the warning.
PEDANTRY_EXCEPTION = no-error=

# Clang supports some specific errors we cannot enable on GCC.
PEDANTIC_FLAGS += -Werror=covered-switch-default
PEDANTIC_FLAGS += -Werror=missing-variable-declarations
PEDANTIC_FLAGS += -Werror=comma
PEDANTIC_FLAGS += -Werror=shorten-64-to-32
PEDANTIC_FLAGS += -Werror=disabled-macro-expansion
