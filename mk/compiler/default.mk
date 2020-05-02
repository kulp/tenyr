# Our default way to handle exceptions to fatal warnings is to suppress them
# entirely. Some compilers (see clang.mk) can raise a non-fatal warning
# instead.
PEDANTRY_EXCEPTION = no-
