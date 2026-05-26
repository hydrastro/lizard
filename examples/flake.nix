{
  description = "Development shells for the dynsys TPCAS + Dear ImGui visualizer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    imgui = {
      url = "github:ocornut/imgui/v1.92.8";
      flake = false;
    };
  };

  outputs = { nixpkgs, imgui, ... }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          winPkgs = pkgs.pkgsCross.mingwW64;
          winTarget = winPkgs.stdenv.hostPlatform.config;
        in {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              clang
              clang-tools
              gdb
              gnumake
              pkg-config
            ];

            buildInputs = with pkgs; [
              cglm
              glew
              glfw3
              libGL
              xorg.libX11
              xorg.libXcursor
              xorg.libXi
              xorg.libXinerama
              xorg.libXrandr
            ];

            CC = "clang";
            CXX = "clang++";
            IMGUI_DIR = "${imgui}";

            shellHook = ''
              export CC=clang
              export CXX=clang++
              echo "dynsys Dear ImGui native dev shell"
              echo "  build: make"
              echo "  run:   make run"
              echo "  clean: make clean"
              echo "  IMGUI_DIR=$IMGUI_DIR"
            '';
          };

          windows = winPkgs.mkShell {
            strictDeps = true;

            nativeBuildInputs = [
              pkgs.gnumake
              pkgs.pkg-config
              winPkgs.stdenv.cc
            ];

            buildInputs = [
              winPkgs.cglm
              winPkgs.glew
              winPkgs.glfw3
            ];

            IMGUI_DIR = "${imgui}";
            PKG_CONFIG_ALLOW_CROSS = "1";

            CC = "${winTarget}-gcc";
            CXX = "${winTarget}-g++";
            AR = "${winTarget}-ar";
            WIN_TRIPLE = winTarget;
            WIN_CC = "${winTarget}-gcc";
            WIN_CXX = "${winTarget}-g++";
            WIN_AR = "${winTarget}-ar";
            WIN_PKG_CONFIG = "pkg-config";

            shellHook = ''
              export CC=${winTarget}-gcc
              export CXX=${winTarget}-g++
              export AR=${winTarget}-ar
              export WIN_TRIPLE=${winTarget}
              export WIN_CC=${winTarget}-gcc
              export WIN_CXX=${winTarget}-g++
              export WIN_AR=${winTarget}-ar
              export WIN_PKG_CONFIG=pkg-config
              echo "dynsys Windows cross-build shell"
              echo "  target: ${winTarget}"
              echo "  build:  make windows"
              echo "  output: build/windows/dynsys.exe"
              echo "  IMGUI_DIR=$IMGUI_DIR"
            '';
          };
        });

      formatter = forAllSystems (system:
        let pkgs = import nixpkgs { inherit system; };
        in pkgs.nixfmt-rfc-style);
    };
}
