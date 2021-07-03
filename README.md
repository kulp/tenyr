# tenyr
[![C-language CI](https://github.com/kulp/tenyr/workflows/C-language%20CI/badge.svg)](https://github.com/kulp/tenyr/actions?query=workflow%3A%22C-language+CI%22)
[![Coverage Status](https://img.shields.io/codecov/c/github/kulp/tenyr.svg)](https://codecov.io/github/kulp/tenyr)

## Overview

**tenyr** is a 32-bit computer architecture and computing environment that
focuses on simplicity of design and implementation. **tenyr**'s tools run on
Mac, \*nix, and Windows on multiple architectures. **tenyr**'s highly portable
Verilog hardware definition has been demonstrated on a Lattice ECP5 FPGA with
the [ULX3S] development board, and on a Xilinx Spartan6 FPGA with the [Nexys3]
development board.

**tenyr** comprises :

* an [instruction set architecture (ISA)](https://github.com/kulp/tenyr/wiki/Assembly-language)
* an [implementation in FPGA hardware](https://github.com/kulp/tenyr/tree/develop/hw/verilog) with device support
  * VGA text output at 64x32 resolution is supported
  * no input devices are currently implemented in hardware &ndash; help appreciated !
* tools for building software
  * [assembler (tas)](https://github.com/kulp/tenyr/wiki/Assembler)
  * [linker (tld)](https://github.com/kulp/tenyr/wiki/Linker)
  * [simulator (tsim)](https://github.com/kulp/tenyr/wiki/Simulator)
* a [standard library](https://github.com/kulp/tenyr/tree/develop/lib) of tenyr code
* some [example software](https://github.com/kulp/tenyr/tree/develop/ex), including :
  * [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) ([tenyr source code](https://github.com/kulp/tenyr/blob/develop/ex/bm_conway.tas))
  * random snakes "screensaver" ([tenyr source code](https://github.com/kulp/tenyr/blob/develop/ex/bm_snake.tas) &ndash; [running in the simulator](https://vimeo.com/98338696), [running on the FPGA](https://vimeo.com/103773300))
  * a [recursive Fibonacci number generator](https://github.com/kulp/tenyr/blob/develop/ex/bm_fib.tas)

**tenyr**'s documentation is [a wiki](https://github.com/kulp/tenyr/wiki), and it keeps a [changelog](Changelog.md) from v0.9.4 onward.

[ULX3S]: https://ulx3s.github.io
[Nexys3]: https://reference.digilentinc.com/programmable-logic/nexys-3/
