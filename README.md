# tenyr
[![Build Status](https://travis-ci.org/kulp/tenyr.svg?branch=develop)](https://travis-ci.org/kulp/tenyr)
[![Coverage Status](https://img.shields.io/codecov/c/github/kulp/tenyr.svg)](https://codecov.io/github/kulp/tenyr)

## Overview

**tenyr** is a 32-bit computer architecture and computing environment that
focuses on simplicity of design and implementation. **tenyr**'s tools run on
Mac, \*nix, Windows, node.js (via [emscripten](https://github.com/kripken/emscripten)),
and [in a web browser](http://demo.tenyr.info/). **tenyr**'s highly portable
Verilog hardware definition has been demonstrated on Xilinx Spartan6 FPGAs and
should run fine on many FPGAs while using less than 1200 LUT6-equivalents.

**tenyr** comprises :

* an [instruction set architecture (ISA)](https://github.com/kulp/tenyr/wiki/Assembly-language)
* an [implementation in FPGA hardware](https://github.com/kulp/tenyr/tree/develop/hw/verilog) with device support
  * VGA text output at 80x25 resolution is supported
  * no input devices are currently implemented in hardware &ndash; help appreciated !
* tools for building software
  * [assembler (tas)](https://github.com/kulp/tenyr/wiki/Assembler)
  * [linker (tld)](https://github.com/kulp/tenyr/wiki/Linker)
  * [simulator (tsim)](https://github.com/kulp/tenyr/wiki/Simulator)
* a [standard library](https://github.com/kulp/tenyr/tree/develop/lib) of tenyr code
* some [example software](https://github.com/kulp/tenyr/tree/develop/ex), including :
  * [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) ([tenyr source code](https://github.com/kulp/tenyr/blob/develop/ex/bm_conway.tas.cpp))
  * random snakes "screensaver" ([tenyr source code](https://github.com/kulp/tenyr/blob/develop/ex/bm_snake.tas.cpp) &ndash; [running in the simulator](https://vimeo.com/98338696), [running on the FPGA](https://vimeo.com/103773300))
  * a [recursive Fibonacci number generator](https://github.com/kulp/tenyr/blob/develop/ex/bm_fib.tas.cpp)

Someday it will also include :

* [a Forth environment](https://github.com/kulp/tenyr/tree/develop/forth) (work in progress)
* a [C compiler](https://github.com/kulp/lcc-tenyr) based on [LCC](https://github.com/drh/lcc)
* a novel cooperative real-time operating system

**tenyr**'s documentation is [a wiki](https://github.com/kulp/tenyr/wiki).
Feel free to join our [Gitter IM room](https://gitter.im/kulp/tenyr).
