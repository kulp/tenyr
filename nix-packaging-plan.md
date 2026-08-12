# Nix packaging: evaluation & improvement plan

**Status: build-verified on the `nix` branch.** `nix build .#tenyr`,
`nix build .#tenyr-tests`, `nix flake check`, and the legacy `nix-build -A tenyr`
all succeed. The built `tas`/`tld`/`tsim` are smoke-tested from `$out`.

> Prefer the **flake**: `nix build .#tenyr` (or `nix run .#tsim`). This host
> does not enable `nix-command`/`flakes` by default (`nix.conf` only sets
> `build-users-group = nixbld`), so invoke with
> `--extra-experimental-features nix-command --extra-experimental-features flakes`,
> or enable them in `~/.config/nix/nix.conf`.

## Environment

- Host: Nix 2.35.1, macOS 15 (arm64), multi-user (`nixbld`).
- nixpkgs: `nixos-unstable`, locked in `flake.lock` to
  `279b4a8275f032c566576b3f181fa0f27197f588` (mirrored by `default.nix`).
  `nixos-23.11` is **not** used -- it has no cached Darwin bootstrap binaries,
  forcing a multi-hour clang source build.
- iverilog: **13.x** (nixos-unstable). iverilog-13 is stricter than 12 (see
  "Source fix" below).
- No nix channels are configured (legacy `nix-build`/`nix-instantiate`).

## What the latest commit added (vs `c8f8e6ad`)

`c8f8e6ad` ("Introduce initial Nix packaging") added a hand-rolled `default.nix`
(nixos-23.11, hashless `fetchTarball`, `src` from the GitHub `v1.0.0-rc2`
release, all deps in `buildInputs`, no `meta`, no check phase). The current
`nix` tree adds:

- **`flake.nix` + `flake.lock`** -- flake entry: locked nixpkgs,
  `forSystems = [ x86_64-linux aarch64-darwin x86_64-darwin ]`, `packages`
  (`default`/`tenyr`/`tenyr-tests`) and `apps.default` (`tsim`).
- **`tenyr.nix`** -- rewritten as idiomatic `stdenv.mkDerivation`:
  `pname`/`version`, `nativeBuildInputs` (build-time: cmake, bison, flex,
  iverilog) vs `buildInputs` (linked: SDL2, SDL2_image), `meta`, and new params
  `src`, `iverilog`, `doTests`, `doCheck`.
- **`hw/verilog/tenyr.v`** -- one-line source fix required by iverilog-13.
- **`default.nix`** -- modernized to mirror the flake (legacy `nix-build` entry).

## Results of running Nix tooling (verified this session)

1. **`nix build .#tenyr`** -- exit 0. Builds `tenyr-src` + `tenyr-0.9.9` (2 drvs).
   Install layout: `$out/bin/{tas,tld,tsim}`, `$out/lib/{libtenyrsdlvga.so,
   libtenyrsdlled.so, vpidevices.vpi}`.
2. **`nix build .#tenyr-tests`** -- exit 0; ctest is gated (`doCheck = doTests`)
   and passes (`100% / 608`, see **Tests**) -- configures+builds with `-DTESTING=1`.
3. **`nix flake check`** -- "all checks passed" (packages `tenyr` +
   `tenyr-tests`, app `tsim`; only a benign warning that the app lacks `meta`).
4. **`nix-build -A tenyr`** (legacy `default.nix`) -- exit 0; same install layout.
5. **tsim/tas smoke** (from the built `$out`):
   - `tas test/misc/deref.tas -o deref.texe` -> exit 0.
   - `tsim -v -ftext -` (stdin, empty program) -> exit 0, prints
     `IP = 0x00001000` (matches `tests.cmake:364` PASS_REGULAR_EXPRESSION).
   - `tsim -rzero_word deref.texe` -> exit 0 (the "deref zero success" case).
     Note: `tsim deref.texe` with no args is, by design in `tests.cmake:403`, a
     `WILL_FAIL TRUE` test -- exit 1 there is the expected pass.
   - The SDL plugins (`libtenyrsdlvga.so`, `libtenyrsdlled.so`) and the VPI module
     `vpidevices.vpi` are installed to `$out/lib`, so `tsim`'s `dlopen` of the
     sdlvga/sdlled plugins resolves to the nix store (the runtime-plugin
     question that was open in the prior note is resolved).

## Evaluation

### What is good
- `src = ./.` (working tree) with a `runCommand` graft that splices the pinned
  `3rdparty/wb_intercon` (rev `3c5d2fc...`) via `builtins.fetchGit`. Nix's git
  source filter does not expand submodules and a path literal into an
  unexpanded submodule hits "not tracked by Git" -- the graft sidesteps both.
  (`wiki` and `ulx3s` submodules are docs/KiCad tooling, not referenced by the
  cmake build, and are excluded from the legacy `src`.)
- The `SDL2_image` `find_package` breakage (its CMake config calls
  `find_dependency(webp/TIFF)`, but nixpkgs ships SDL2_image without propagating
  them) is fixed with an overlay -- `propagatedBuildInputs = old.buildInputs` --
  in both `flake.nix` and `default.nix`.
- The two-stage `tas` packaging works: `doTests=true` passes the base-built
  `tas`/`tld`/`tsim` as `CMAKE_TENYR_COMPILER`/`-LINKER`/`TENYR_SIMULATOR`,
  satisfying `enable_language(TENYR)` at configure (proving the tas toolchain
  packages and is discoverable).
- Idiomatic attrs: `pname = "tenyr"; version = "0.9.9"` (reconciled with the
  `0.9.9` in `CMakeLists.txt`), `meta` with license/homepage/description.
- A `forAllSystems` flake + locked nixpkgs makes the build reproducible and
  hits cached Darwin substitutes.

### Source fix (required, in-scope for the build)

`hw/verilog/tenyr.v` (`Core` module) declared
`wire nextI = branching ? nextZ : nextP;` **before** `wire nextZ = ...`.
iverilog-12 (nixos-23.11) accepts the forward reference; **iverilog-13 errors**
during elaboration, which aborts the `-DICARUS=1` build:
```
tenyr.v:102: error: Unable to bind wire/reg/memory `nextZ' in `Top.tenyr.core'
tenyr.v:102: error: Unable to elaborate r-value: (branching)?(nextZ):(nextP)
2 error(s) during elaboration.
```
Fix: declare `nextZ` before `nextI`. Semantically identical (both are
continuous `wire` assignments, order-independent). Verified by building with the
original (fails, exit 1, as above) and with the fix (green, exit 0).

### Remaining issues (non-JIT)
1. **iverilog-13 "no explicit time unit" notice** from the vendored
   `3rdparty/wb_intercon` Verilog (it carries no `` `timescale ``). It is a
   **warning, non-fatal**, and originates in the submodule (not tnyr). Options:
   graft a `` `timescale `` wrapper, patch wb_intercon, or pin iverilog 12.
2. **`default.nix` source filtering**: legacy `nix-build`'s path literal does
   **not** gitignore-filter the working tree, so a stale `build/CMakeCache.txt`
   (left by an in-tree cmake from the `/Users/kulp/tenyr` symlink) made cmake
   refuse to configure ("source does not match the cache"). Fixed in
   `default.nix` with a `builtins.path` filter that drops `build/`, `tsim`,
   `vsim`, submodules, `*.swp`, `*.o`, `*.texe`, `*.d`, `*.to`. The flake is
   unaffected (flake eval's `${./.}` honors `.gitignore`, which already lists
   `build/`).

## Tests

`tests.cmake` registers ctest against every op/program (assemble with `tas`,
simulate with `tsim` and `vvp`, compare stdout/stderr to the committed golden
files under `test/compare/{tas,tsim,vvp}/{in,out,err}/*`), plus tsim-plugin,
SDL, and IRC cases -- **608 tests total**.

**ctest is now gated** (`doCheck = doTests`). With `doCheck=true`, ctest on the
rebased nix/iverilog-13 build reports **`100% passed -- 0 failed out of 608`**
(608 pass). The two pre-rebase failure cohorts are both resolved:
- **`vvp_*_icarus_compare_stdout`** (was ~48 failed) -- already green at **86/86**
  before any plugin change: rebase onto `develop`(`f85ec698`) brought in
  `develop`'s `hw/verilog/tenyr.v` history, which brings iverilog-13's `vvp` dump
  output back in line with the **unchanged** committed goldens under
  `test/compare/vvp/out/*`. The drift was a **source** skew on the old nix-side
  `tenyr.v` (pre-`develop`'s TMDS/naming work), not a golden/iverilog mismatch:
  iverilog-13's output now matches the goldens byte-for-byte, so **no golden
  regeneration and no iverilog-12 pin** was needed.
- **`tsim_plugin_*`** (was 7: `no_error_create`, `operation_compare_stderr`,
  `initialisation_compare_stderr`, `finalisation_compare_stderr`,
  `nested_init_compare_stderr` + 2 Not-Run dependents) -- resolved by adding an
  `install(TARGETS tenyrfailure_* ... DESTINATION lib)` rule to
  `src/devices/CMakeLists.txt` (mirrors the existing `tenyrsdlled`/`tenyrsdlvga`
  install). tsim runs at ctest from `${tnyrAsm}/bin/tsim` (the nix store) and
  searches `../lib/` first (the `library_search_paths = ["../lib/", "./", ""]` in
  `src/tsim.c`); with `tenyrfailure_*` now in `$out/lib`, tsim `dlopen`s them and
  the cohort passes. iverilog-independent.
`tenyr-tests` still configures with `-DTESTING=1` (so `enable_language(TENYR)` and
the tas toolchain resolve and the icarus sims + test plugins build) and, via
`doCheck = doTests`, now runs the full ctest suite (100%/608). `nix build .#tenyr`
(default, `doTests=false`, `doCheck=false`) builds the tools without ctest.

## JIT (deferred)

`-DJIT=1` is **not** built. nixpkgs' `lightning` is `broken` on Darwin and ships
**2.2.2 as shared-only**; tnyr pins **2.1.3** (`scripts/build-lightning.sh`) as a
**static** `liblightning.a`, and `jit.c` uses the 2.1.x global-`_jit` ABI -- so
linking fails with undefined symbols (`_init_jit`, `__jit_new_state`,
`__jit_new_node_*`, `__jit_prepare`, `__jit_prolog`, ...). To enable: vendor
lightning 2.1.3 as a static derivation -> add to `nativeBuildInputs` -> `-DJIT=1`
behind a variant. Not done (macOS Lightning not feasible via nixpkgs).

## Improvement plan / next

1. [x] flake + idiomatic `tenyr.nix` + `src` graft + `SDL2_image` overlay + meta.
2. [x] `tenyr.v` `nextZ`-before-`nextI` reorder (iverilog-13 build blocker).
3. [x] `default.nix` mirrors the flake (legacy `nix-build` entry green).
4. [x] Re-enable ctest (done): install `tenyrfailure_*` to `$out/lib`
   (`src/devices/CMakeLists.txt`, mirroring the SDL plugins) so `tsim` finds
   them; `doCheck = doTests`. ctest: `100% / 608`, 0 failures.
5. (optional) Suppress/patch the wb_intercon "no explicit time unit" notice.
6. (deferred) JIT via vendored static lightning 2.1.3.

## Out of scope
- `hw/verilator/`, `tsim`, `vsim`, `build/` are untracked dev-tree artifacts
  (not part of the nix build). `build/`, `*.o`, `*.texe`, `*.to`, `*.d` are in
  `.gitignore`; the rest are stripped from the legacy `src` via the
  `builtins.path` filter and excluded from the flake's `${./.}` by gitignore.
- `wiki` / `3rdparty/ulx3s` submodules: docs / KiCad tooling, not referenced by
  the cmake build.

## Verification (post-rebase onto `develop`)

After rebasing `nix` onto `develop` (`f85ec698`), re-verified green on the
rebased tree (`nix` @ `8fcd6c8b`): `tenyr.v` is already fixed on develop via
`cf85f4a2` (identical to the nix-side fix); the packaging files
(flake.nix/lock, tnyr.nix, default.nix, this plan) are nix-only additions that
develop does not carry, so no packaging changes were required by the rebase.
- `nix build .#tenyr` -- exit 0 (rebuilt `tenyr-src` + `tenyr-0.9.9`).
- `nix build .#tenyr-tests` -- exit 0.
- `nix flake check` -- "all checks passed!" (benign: app lacks `meta`).
- `nix-build -A tenyr` -- exit 0 (full clang + iverilog-13 build; the wb_intercon
  "no explicit time unit" notice is non-fatal).
- Smoke: `tas deref.tas -> /tmp/deref.texe` (exit 0); `tsim -rzero_word` (exit 0);
  `tsim -v -ftext -` prints `IP = 0x00001000`.
