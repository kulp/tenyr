{
  description = "tenyr: a 32-bit computer architecture, ISA, simulator and tools";

  inputs = {
    # A current nixpkgs so Darwin substitutes are available: the old
    # nixos-23.11 pin has no cached bootstrap binaries left on the binary
    # cache, which forced a multi-hour LLVM/clang source build.
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      inherit (nixpkgs) lib;
      systems = [ "x86_64-linux" "aarch64-darwin" "x86_64-darwin" ];
      forSystem = system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [
              # SDL2_image's installed CMake config unconditionally requires
              # its image deps (webp, TIFF) via find_dependency(), but nixpkgs
              # ships SDL2_image without propagating them -- so consumers like
              # tenyr fail `find_package(SDL2_image)`. Propagate them.
              (self: super: {
                SDL2_image = super.SDL2_image.overrideAttrs (old: {
                  propagatedBuildInputs = old.buildInputs;
                });
              })
            ];
          };
          # Build the *current* working-tree source (git-cleaned: excludes
          # untracked build/ tsim vsim and gitignored *.o *.texe, so no stale
          # CMakeCache) with the wb_intercon git submodule grafted in -- Nix's
          # git source filter does not expand submodules and path literals into
          # a submodule hit "not tracked by Git", so we graft the submodule's
          # fetched tree explicitly. (ulx3s = KiCad PCB tooling and wiki = docs
          # are not referenced by the cmake build.)
          wb_intercon = builtins.fetchGit {
            url = "https://github.com/olofk/wb_intercon.git";
            rev = "3c5d2fc7e6e04f75b4a48cca941d543e7c160e2d";
          };
  src = pkgs.runCommand "tenyr-src" { } ''
    mkdir -p $out
    cp -a ${./.}/* $out/
    # `${./.}` does not reliably drop the gitignored dev-tree artifacts
    # (build/, stray tsim/vsim) -- flake eval excludes them, legacy nix-build
    # does not. Strip explicitly: a stale build/CMakeCache.txt left by an
    # in-tree cmake run (from the /Users/kulp/tenyr symlink) makes cmake refuse
    # to configure ("source does not match the cache").
    chmod -R u+w $out/build "$out/tsim" "$out/vsim" 2>/dev/null || true
    rm -rf $out/build
    rm -f "$out"/tsim "$out"/vsim
    # ${./.} may include a locally checked-out 3rdparty/wb_intercon (at an
    # arbitrary rev, copied read-only). Re-graft the pinned rev
    # idempotently: chmod first (read-only store copy blocks overwrite),
    # drop the copied tree, then install the pinned one.
    chmod -R u+w $out/3rdparty 2>/dev/null || true
    rm -rf $out/3rdparty/wb_intercon
    mkdir -p $out/3rdparty/wb_intercon
    cp -a ${wb_intercon}/. $out/3rdparty/wb_intercon/
  '';
          # Base package: builds the tools (incl. `tas`) with TESTING off.
          tenyr = pkgs.callPackage ./tenyr.nix { inherit src; };
          # Test build: TESTING on + `tas` from the base package in PATH so
          # `enable_language(TENYR)` can find the assembler at configure time.
        in
        {
          packages = {
            default = tenyr;
            tenyr = tenyr;
            tenyr-tests = pkgs.callPackage ./tenyr.nix {
              src = src;
              doTests = true;
              tnyrAsm = tenyr;
            };
          };
          apps = {
            default = { type = "app"; program = "${tenyr}/bin/tsim"; };
          };
        };
    in
    {
      packages = lib.genAttrs systems (s: (forSystem s).packages);
      apps = lib.genAttrs systems (s: (forSystem s).apps);
    };
}
