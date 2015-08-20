# tenyr
[![Build Status](https://travis-ci.org/kulp/tenyr.svg?branch=develop)](https://travis-ci.org/kulp/tenyr)
[![Coverage Status](https://img.shields.io/codecov/c/github/kulp/tenyr.svg)](https://codecov.io/github/kulp/tenyr)
[![Coverity Scanned](https://img.shields.io/coverity/scan/5645.svg)](https://scan.coverity.com/projects/5645)
[![Latest Tag](https://img.shields.io/github/tag/kulp/tenyr.svg)](https://github.com/kulp/tenyr/releases)
[![Free Software](https://img.shields.io/github/license/kulp/tenyr.svg)](https://github.com/kulp/tenyr/blob/develop/COPYING)

## Overview

**tenyr** is a 32-bit computer architecture and computing environment that
focuses on simplicity of design and implementation. **tenyr** comprises :

* an [instruction set architecture (ISA)](https://github.com/kulp/tenyr/wiki/Assembly-language)
* an [implementation in FPGA hardware](https://github.com/kulp/tenyr/tree/develop/hw/verilog) with device support
* tools for building software
  * [assembler (tas)](https://github.com/kulp/tenyr/wiki/Assembler)
  * [linker (tld)](https://github.com/kulp/tenyr/wiki/Linker)
  * [simulator (tsim)](https://github.com/kulp/tenyr/wiki/Simulator)
* a [standard library](https://github.com/kulp/tenyr/tree/develop/lib) of tenyr code
* some [example software](https://github.com/kulp/tenyr/tree/develop/ex), including :
  * a [timer](https://github.com/kulp/tenyr/blob/develop/ex/clock.tas.cpp)
  * [random snakes](https://github.com/kulp/tenyr/blob/develop/ex/bm_snake.tas.cpp) ([running in the simulator](https://vimeo.com/98338696), [running on the FPGA](https://vimeo.com/103773300))
  * a [recursive Fibonacci number generator](https://github.com/kulp/tenyr/blob/develop/ex/bm_fib.tas.cpp)

Someday it will also include :

* [a Forth environment](https://github.com/kulp/tenyr/tree/develop/forth) (work in progress)
* a C compiler based on LCC
* a novel cooperative real-time operating system

**tenyr**'s documentation can be found in [its
wiki](https://github.com/kulp/tenyr/wiki). The wiki is far from complete but
constitutes the best available starting point for understanding **tenyr**. The
author would be delighted to answer questions about **tenyr** directed to [his
email](mailto:darren@kulp.ch).

