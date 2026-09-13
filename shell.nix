with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "decker-build-env";
    buildInputs = [ SDL2 SDL2_image ];
}
