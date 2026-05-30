{
  description = "lizard wizard";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    ds.url = "github:hydrastro/ds";
  };

  outputs = { self, nixpkgs, ds }:
    let
      systems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      dsFor = system:
        if ds.packages.${system} ? default then ds.packages.${system}.default
        else if ds.packages.${system} ? ds then ds.packages.${system}.ds
        else ds.defaultPackage.${system};
    in {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          dsPackage = dsFor system;
        in rec {
          lizard = pkgs.stdenv.mkDerivation {
            pname = "lizard";
            version = "0.0.0";
            src = ./.;

            nativeBuildInputs = [ pkgs.gnumake ];
            buildInputs = [ pkgs.gmp dsPackage ];

            buildPhase = ''
              make
            '';

            installPhase = ''
              make install PREFIX=$out
            '';

            meta = with pkgs.lib; {
              description = "Lizard Lisp wizard; a Scheme interpreter library written in C89";
              platforms = platforms.unix;
            };
          };

          default = lizard;
        });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          dsPackage = dsFor system;
        in {
          default = pkgs.mkShell {
            packages = [
              pkgs.gcc
              pkgs.gnumake
              pkgs.gdb
              pkgs.valgrind
              pkgs.gmp
              dsPackage
            ];
          };
        });

      defaultPackage = forAllSystems (system: self.packages.${system}.default);
    };
}
