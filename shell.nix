with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "decker-build-env";
    buildInputs = [ unixtools.xxd SDL2 SDL2_image ];
}
