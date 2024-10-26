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
            mkdir -p $out
            gcc -c lizard.c -lgmp -o lizard
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp lizard $out/bin
          '';
          postInstall = ''
            cp $out/bin/lizard ./lizard
          '';

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