{
  description = "cosmic";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
  };

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      lib = nixpkgs.lib;

      libDeps = with pkgs; [
        mesa
        vulkan-loader
        wayland
      ];
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        LD_LIBRARY_PATH = lib.makeLibraryPath libDeps;
        nativeBuildInputs = with pkgs; [
          cmake ninja pkg-config wayland wayland-scanner wayland-protocols vulkan-validation-layers
          libx11 libxext xorg.libxcb.dev libxkbcommon egl-wayland libGL
          libpng zlib bzip2 brotli
          libbacktrace sdl3 sdl3-image
          file # This is required by libbacktrace's configure script. TODO add a libbacktrace target to cmake
        ];
      };
    };
}