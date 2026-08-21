{
  description = "A nix flake for c projects and environment";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-25.11";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
  outputs =
    {
      flake-parts,
      ...
    }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "x86_64-linux"
      ];
      flake = {
        templates.default.path = ./.;
      };
      perSystem =
        {
          config,
          self',
          inputs',
          pkgs,
          system,
          ...
        }:
        let
          nativeBuildInputs = with pkgs; [ ];
          buildInputs = with pkgs; [
            libc
            freeglut
            libglut
            pkgs.llvmPackages.openmp

            glfw
            glew
            mesa
            libGL
            libglvnd
            libGLU
          ];

          bin = "clang_base_dev_flake";

          pythonPackages =
            ps: with ps; [
              opencv-python-headless
              numpy
            ];
          pythonEnv = pkgs.python3.withPackages pythonPackages;
        in
        {
          packages.default = pkgs.clangStdenv.mkDerivation {
            inherit buildInputs nativeBuildInputs;
            name = "${bin}";
            src = ./.;
            BIN_NAME = bin;
            buildPhase = ''
              runHook preBuild
              export BIN_NAME=${bin}
              make dir
              make build
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin
              cp -ra bin/* $out/bin
              runHook postInstall
            '';
          };

          devShells.default = pkgs.mkShell {
            inherit nativeBuildInputs buildInputs;
            packages = with pkgs; [
              clang-tools
              lldb
              compiledb
              autotools-language-server

              python3
              pyright
              pythonEnv
            ];
            BIN_NAME = bin;
          };
        };
    };
}
