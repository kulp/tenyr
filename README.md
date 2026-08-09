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

## Building for the web browser

**tenyr** can be compiled to [WebAssembly](https://webassembly.org/) via
[Emscripten](https://emscripten.org/) so that the simulator runs entirely in a
web browser.  The assembler (`tas`) and linker (`tld`) are also built as WebAssembly
modules and can be used from Node.js to produce binaries.

### Prerequisites

* [Emscripten SDK](https://emscripten.org/docs/gettingstarted/builds-and-tests.html) (`emcc`, `emcmake`)
* CMake ≥ 3.19
* Node.js (for testing the wasm tools from the command line)
* bison ≥ 3.7.6, flex

### Build & test

```sh
make wasm-wasm        # configure + build tas.js, tld.js, tsim.js
make wasm-tools       # assemble + link demo programs, copy to ui/web/
```

### Running in a browser

Serve the `ui/web/` directory with any static file server and open
`index.html`:

```sh
cd ui/web && python3 -m http.server 8080
```

Then visit `http://localhost:8080` in a browser.  Select a program and click
**Run**.  The simulator uses the `emscript` recipe, which runs the event loop
via `emscripten_set_main_loop_arg` so the browser stays responsive.

### How it works

The emscripten build reuses the same C source code as the native build, with
an OS-specific abstraction layer in `src/os/Emscripten/`.  Key pieces:

| File | Purpose |
|------|---------|
| `src/os/Emscripten/preamble.c` | `os_preamble()` — sets up `/dev/zero` and mounts the host filesystem under Node.js |
| `src/os/Emscripten/open.c` | `os_fopen()` — rewrites absolute paths to the mount point |
| `src/os/Emscripten/findself.c` | `os_find_self()` — returns `.` (no real executable path) |
| `src/os/Emscripten/emscripten.c` | `recipe_emscript()` — event-loop recipe using `emscripten_set_main_loop_arg` |
| `src/tsim.c` | `"emscript"` recipe added to the recipe book; `#include <unistd.h>` for `usleep()` |
| `src/os/default/emscripten.c` | Stub `recipe_emscript()` for native builds (fatals if used) |

[ULX3S]: https://ulx3s.github.io
[Nexys3]: https://reference.digilentinc.com/programmable-logic/nexys-3/
