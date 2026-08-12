{ lib
, stdenv
, fetchFromGitHub
, cmake
, bison
, flex
, SDL2
, SDL2_image
, iverilog
, tnyrAsm ? null
, src ? fetchFromGitHub {
    owner = "kulp";
    repo = "tenyr";
    rev = "v1.0.0-rc2";
    fetchSubmodules = true;
    sha256 = "y5R6ttyi0g2StMWX+fPCmyzwRxPKNo5tEWfD3JM2l/w=";
  }
, doTests ? false
}:

stdenv.mkDerivation {
  pname = "tenyr";
  version = "0.9.9";

  inherit src;

  # Build tools run on the build platform; SDL stays on the host (linked).
  nativeBuildInputs = [
    cmake
    bison
    flex
    iverilog
  ] ++ lib.optional doTests tnyrAsm;

  buildInputs = [
    SDL2
    SDL2_image
  ];

  # `cmake` in nativeBuildInputs activates the cmake build helper, which
  # drives configure/build/install from `cmakeFlags` (no manual phases needed).
  cmakeFlags = [
    "-DSDL=1"
    "-DICARUS=1"
  ] ++ lib.optionals doTests [
    "-DTESTING=1"
    # enable_language(TENYR) looks for tas/tld/tsim under CMAKE_BINARY_DIR/src
    # (NO_DEFAULT_PATH), not PATH. Hand it the base-built tools directly so the
    # test configure succeeds without a two-stage build.
    "-DCMAKE_TENYR_COMPILER=${tnyrAsm}/bin/tas"
    "-DCMAKE_TENYR_LINKER=${tnyrAsm}/bin/tld"
    "-DTENYR_SIMULATOR=${tnyrAsm}/bin/tsim"
  ];

  # ctest is gated on doCheck = doTests. With doCheck=true, ctest on the
  # nix/iverilog-13 build now reports 100% pass -- 0 of 608 tests fail. The
  # failure_* test plugins are installed to $out/lib via the install rule in
  # src/devices/CMakeLists.txt (mirrors the SDL plugins), so tsm -- run from
  # ${tnyrAsm}/bin/tsim in the nix sandbox -- finds them on its ../lib dl search
  # path and the tsim_plugin_* cohort passes. The vvp_*_icarus_compare_stdout
  # cohort was already green (86/86): iverilog-13 vvp output matches the
  # committed test/compare/vvp/out/* goldens, so no golden regen or iverilog-12
  # pin was needed. See nix-packaging-plan.md.
  doCheck = doTests;

  # FIXME: JIT (-DJIT=1) is disabled. It needs GNU lightning, which nixpkgs
  # marks `broken` on Darwin and whose shared library on Darwin does not link
  # `tenyrjit`'s symbols (undefined: _init_jit, _jit_new_state, __jit_new_node_*,
  # __jit_prepare, __jit_prolog, ...). To enable, vendor lightning 2.1.3 -- the
  # version pinned by scripts/build-lightning.sh -- as a *static* derivation and
  # add it to nativeBuildInputs.
  meta = with lib; {
    description = "tenyr: a 32-bit computer architecture, ISA, simulator and tools";
    homepage = "https://github.com/kulp/tenyr";
    license = licenses.mit;
    platforms = platforms.unix;
  };
}
