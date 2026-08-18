{
    lib,
    stdenv,
    fetchFromGitHub,
    cmake,
    bison, flex, SDL2, SDL2_image, verilog
}:

stdenv.mkDerivation {
    name = "tenyr";
    version = "v1.0.0-rc2";
    src = fetchFromGitHub {
        owner = "kulp";
        repo = "tenyr";
        rev = "v1.0.0-rc2";
        fetchSubmodules = true;
        sha256 = "y5R6ttyi0g2StMWX+fPCmyzwRxPKNo5tEWfD3JM2l/w=";
    };

    buildInputs = [
        cmake
        bison
        flex
        SDL2
        SDL2_image
        verilog
    ];

    # TODO build with -DJIT=1 once lightning is not a broken package:
    configurePhase = "cmake -DSDL=1 -DICARUS=1 -S . -B build";
    buildPhase = "cmake --build build";
    installPhase = ''cmake --install build --prefix "$out"'';
}
