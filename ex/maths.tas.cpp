#include "common.th"

start:
  prologue

  c <- 11
  d <- 5
  call(umod)

  c <- 50
  d <- 10
  call(udiv)

  c <- 36
  call(isqrt)

  b <- [@+arg0 + p]
  c <- [@+arg1 + p]
  d <- [@+arg2 + p]
  e <- [@+arg3 + p]
  call(dw_add)

  d <- [@+arg0 + p]
  e <- [@+arg1 + p]
  call(dw_mul)

  illegal

arg0: .word 0x11223344
arg1: .word 0xaabbccdd
arg2: .word 0xbad0cafe
arg3: .word 0xdeadbeef

