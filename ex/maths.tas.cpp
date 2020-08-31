#include "common.th"

start:
  prologue

  c <- 11
  d <- 5
  push(p + 2); p <- @+umod + p

  c <- 50
  d <- 10
  push(p + 2); p <- @+udiv + p

  c <- 36
  push(p + 2); p <- @+isqrt + p

  b <- [@+arg0 + p]
  c <- [@+arg1 + p]
  d <- [@+arg2 + p]
  e <- [@+arg3 + p]
  push(p + 2); p <- @+dw_add + p

  d <- [@+arg0 + p]
  e <- [@+arg1 + p]
  push(p + 2); p <- @+dw_mul + p

  illegal

arg0: .word 0x11223344
arg1: .word 0xaabbccdd
arg2: .word 0xbad0cafe
arg3: .word 0xdeadbeef

