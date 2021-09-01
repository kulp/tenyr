# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Address collected minor issues (#81)
- Remove emscripten support (#92)
- Address some Verilog lints and weaknesses (#94)
- Collected ctest-provoked fixes (#95)
- Enable GitHub Actions for macos-11 runners (#96)
- Prefer sdl2-config over pkg-config (#82)
- Run `apt-get update` before installing packages from APT (#84)
- Fix crash in SDLVGA on macOS Big Sur 11.2 (#83)
- Remove Xilinx support (#86)
- Introduce collected hardware changes for ulx3s (#88)
- Introduce a yosys flow for ULX3S (#87)
- Build on macOS Big Sur (#85)

## [0.9.8] - 2021-01-01
### Added
- Enabled many compiler warnings under Clang and GCC
- Introduced a stream-based abstraction layer for filesystem operations
- Defined the overshifting behavior for the `@` operator
- Defined the tenyr assembly grammar in ABNF (#60)
- Set up continuous integration using GitHub Actions (#66, #67, #68, #69, #70, #71)

### Changed
- Updated supported emscripten version to 1.38.46
- Reimplemented JIT using GNU lightning
- Stopped relying on customizations to `wb_intercon` for tenyr's Verilog implementation
- Addressed various lint warnings found in tenyr's Verilog
- Tightened lexer rules for numeric constants (#56)
- Cleaned up OS overrides (#61)
- Stopped removing output files (in a racy way) when failure occurs (#63)

### Fixed
- Corrected a long-standing tsim bug when right-shifting by more than 31 bits
- Corrected tsim bugs manifesting on big-endian host machines
- Corrected silent sign conversions, adhering to `-Werror=sign-conversion`
- Prevented disassembly from `obj` from always exiting non-zero (#59)
- Prevented a buffer overrun when too many plugins are specified (#64)
- Fixed some miscellaneous bugs while improving test coverage (#63)
    - Corrected incorrect pointer cast in VPI
    - Propagate errors better instead of swallowing them inadvertently

### Removed
- Stopped suggesting Gitter as a chat option
- Stopped using coveralls.io for code coverage reporting
- Dropped `raw` output format
- Dropped explicit support for backslashes as path component separators (#43)
- Removed support for various deprecated and obsolete functionalities (#47)
    - Dropped support for C-style and C++-style comments in .tas files
    - Removed broken Forth implementation
    - Dropped support for deprecated v0 and v1 object formats
    - Remove unused `@=` syntax sugar
    - Removed web demo
    - Removed bitrotten support for Quartus and Lattice builds
    - Dropped unused submodule for broken lcc port
- Stopped using C preprocessor on `.tas.cpp` files (#45)
- Removed Travis-CI support (#72)
- Removed undocumented settings that allowed continuing after memory errors (#63)
- Removed unused `.option` support

## [0.9.7] - 2019-07-25
### Added
- support for `@symbol - .Llocal` expressions
- a nominal, arbitrary limit of 2048 bytes on symbol lengths in version 2 objects (to promote runtime memory safety)

### Changed
- parser supports bison 3.4.1 by replacing deprecated bison directives

### Fixed
- memory leaks and misuse (e.g. read after free), mostly introduced by version 2 objects

## [0.9.6] - 2018-04-29
### Added
- version 2 objects supporting newly-unlimited symbol lengths

### Changed
- improved emscripten support

## [0.9.5] - 2017-11-16
### Added
- a ~/.tsimrc file (à la `--options=file`) that is loaded by a new default recipe
- a new BRAM-optimized 10x15 font and associated build tools
- `@+var` and `@=var` shorthand forms
- tests for op types 0, 1, and 2
- new Verilog module for VGA output

### Changed
- running recipes in the order they are specified, instead of reverse order
- allowing simulations to continue in the face of device errors
- restoring serial input functionality in a limited way
- changing VGA output to be 64x32 text columns (down from 80x40)

### Fixed
- correcting reported column numbers for syntax errors

### Removed
- third-party VHDL for VGA

## [0.9.4] - 2017-05-04
### Added
- build date to --version output
- insertion of missing trailing slash that was causing resource-loading to fail
- first-class emscripten build (`make PLATFORM=emscripten`)

### Changed
- optimised sparseram somewhat
- drop ancient 24-bit address space stricture

### Removed
- uses of unportable `RTLD_DEFAULT`
- uses of little-used assert()
- obsolescent SPI simulated device

[Unreleased]: https://github.com/kulp/tenyr/compare/v0.9.8...HEAD
[0.9.8]: https://github.com/kulp/tenyr/compare/v0.9.7...v0.9.8
[0.9.7]: https://github.com/kulp/tenyr/compare/v0.9.6...v0.9.7
[0.9.6]: https://github.com/kulp/tenyr/compare/v0.9.5...v0.9.6
[0.9.5]: https://github.com/kulp/tenyr/compare/v0.9.4...v0.9.5
[0.9.4]: https://github.com/kulp/tenyr/compare/v0.9.3...v0.9.4
