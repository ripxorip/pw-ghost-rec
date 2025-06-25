{
  description = "Flake for pw-ghost-rec Linux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/release-25.05";
    flake-utils.url = "github:numtide/flake-utils";
    nix-formatter-pack.url = "github:Gerschtli/nix-formatter-pack";
    nix-formatter-pack.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, nix-formatter-pack, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        ghostRecPkg = pkgs.stdenv.mkDerivation {
          pname = "pw-ghost-rec";
          version = "0.1.0";
          src = ./.;
          nativeBuildInputs = [ pkgs.meson pkgs.ninja pkgs.pkg-config ];
          buildInputs = [
            pkgs.pipewire.dev
            pkgs.liblo
            pkgs.check
            pkgs.libsndfile.dev
            pkgs.libmicrohttpd.dev
          ];
          mesonBuildDir = "_out";
        };
      in {
        formatter = pkgs.nixpkgs-fmt;
        devShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            gcc
            pkg-config
            pipewire.dev
            liblo
            meson
            ninja
            check
            libsndfile.dev
            libmicrohttpd.dev
          ];
        };
        packages.default = ghostRecPkg;
        apps.default = flake-utils.lib.mkApp {
          drv = ghostRecPkg;
        };
      }
    );
}