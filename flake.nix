{
  description = "lizard wizard";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    ds.url = "github:hydrastro/ds";
  };

  outputs = { self, nixpkgs, ds }: {
    packages = nixpkgs.lib.genAttrs [ "x86_64-linux" ] (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in rec {
        lizard = pkgs.stdenv.mkDerivation {
          pname = "lizard";
          version = "0.0.0";

          src = ./.;

          # (pkgs.callPackage ds { })
          buildInputs = [ pkgs.gmp pkgs.stdenv.cc ds.defaultPackage.x86_64-linux ];

          buildPhase = ''
            mkdir -p $out/include
            mkdir -p $out/lib
            gcc -c lizard.c -lgmp -o lizard.o
            ar rcs $out/lib/liblizard.a lizard.o
            gcc -shared -o $out/lib/liblizard.so lizard.o
          '';

          installPhase = ''
            cp lizard.h $out/include/
          '';
          #postInstall = ''
          #  cp $out/bin/lizard ./lizard
          #'';

          meta = with pkgs.lib; {
            description = "lizard wizard";
            #license = licenses.mit;
            #maintainers = [ maintainers.yourname ];
            platforms = platforms.unix;
          };
        };
      });

    defaultPackage = { x86_64-linux = self.packages.x86_64-linux.lizard; };

  };
}
