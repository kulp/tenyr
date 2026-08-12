# default.nix -- legacy `nix-build` entrypoint.
#
# The flake is the primary entrypoint: `nix build .#tenyr` / `nix run .#tsim`.
# This mirrors the flake (same nixpkgs pin, SDL2_image overlay, wb_intercon
# graft) so classic `nix-build -A tenyr` builds the same local-tree tnyr.
let
  # Pin to the same nixos-unstable rev as flake.lock (cached Darwin substs;
  # the old nixos-23.11 pin has no cached bootstrap binaries and forced a
  # multi-hour clang source build).
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/archive/279b4a8275f032c566576b3f181fa0f27197f588.tar.gz";
  pkgs = import nixpkgs {
    overlays = [
      (self: super: {
        # SDL2_image's installed CMake config unconditionally requires its
        # image deps (webp, TIFF, ...) via find_dependency(), but nixpkgs ships
        # SDL2_image without propagating them -- so consumers like tenyr fail
        # `find_package(SDL2_image)`. Propagate them.
        SDL2_image = super.SDL2_image.overrideAttrs (old: {
          propagatedBuildInputs = old.buildInputs;
        });
      })
    ];
  };
  # Graft the pinned wb_intercon (Nix's `${./.}` does not expand submodules,
  # and a path literal into an unexpanded submodule hits "not tracked by Git").
  wb_intercon = builtins.fetchGit {
    url = "https://github.com/olofk/wb_intercon.git";
    rev = "3c5d2fc7e6e04f75b4a48cca941d543e7c160e2d";
  };
  # `lib.sourceFiles` (gitignore-aware + explicit filter) instead of the bare
  # path literal: legacy `nix-build` walks submodule checkouts (wiki/,
  # which carry a `.git` file + vim `.swp` swap files) and fails at eval
  # ("opening '/.file': Permission denied"). Exclude the doc/tooling submodules
  # (wiki, ulx3s) and wb_intercon (re-grafted below); .gitignore already drops
  # build/, *.o, *.texe.
  src = builtins.path {
    name = "tenyr-src";
    path = ./.;
    # Exclude submodules (wiki/, ulx3s/ carry .git + vim .swp swap files that
    # legacy nix-build's path walk can't open; wb_intercon is re-grafted below)
    # plus dev artifacts (build/), stray binaries (tsim/vsim), and build outputs
    # (*.o, *.texe, *.d, *.to, *.swp) -- `builtins.path` applies no gitignore.
    filter = path: type:
      let n = baseNameOf path;
      in !(n == ".git" || n == "wiki" || n == "ulx3s" || n == "wb_intercon"
           || n == "build" || n == "tsim" || n == "vsim"
          || pkgs.lib.hasSuffix ".swp" n
          || pkgs.lib.hasSuffix ".o" n || pkgs.lib.hasSuffix ".d" n
          || pkgs.lib.hasSuffix ".texe" n || pkgs.lib.hasSuffix ".to" n);
  };
  # Re-graft wb_intercon at the pinned rev (the local checkout may be absent
  # or at a different commit; the filter above excludes it so this is clean).
  fullSrc = pkgs.runCommand "tenyr-src" { } ''
    mkdir -p $out
    cp -a ${src}/. $out/
    chmod -R u+w $out/3rdparty 2>/dev/null || true
    rm -rf $out/3rdparty/wb_intercon
    mkdir -p $out/3rdparty/wb_intercon
    cp -a ${wb_intercon}/. $out/3rdparty/wb_intercon/
  '';
in
{
  tenyr = pkgs.callPackage ./tenyr.nix { src = fullSrc; };
}
