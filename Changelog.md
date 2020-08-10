# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

## [0.9.5] - 2016-11-16
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

[Unreleased]: https://github.com/kulp/tenyr/compare/v0.9.7...HEAD
[0.9.7]: https://github.com/kulp/tenyr/compare/v0.9.6...v0.9.7
[0.9.6]: https://github.com/kulp/tenyr/compare/v0.9.5...v0.9.6
[0.9.5]: https://github.com/kulp/tenyr/compare/v0.9.4...v0.9.5
[0.9.4]: https://github.com/kulp/tenyr/compare/v0.9.3...v0.9.4
