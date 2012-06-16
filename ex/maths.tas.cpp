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

  b <- [rel(arg0)]
  c <- [rel(arg1)]
  d <- [rel(arg2)]
  e <- [rel(arg3)]
  call(dw_add)

  d <- [rel(arg0)]
  e <- [rel(arg1)]
  call(dw_mul)

  illegal

arg0: .word 0x11223344
arg1: .word 0xaabbccdd
arg2: .word 0xbad0cafe
arg3: .word 0xdeadbeef

