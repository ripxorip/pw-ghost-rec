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
        ghostRecPkg = import ./default.nix { inherit pkgs; };
        reaperPatcherPkg = import ./patchers/REAPER/default.nix { inherit pkgs; };
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
        packages.reaper-patcher = reaperPatcherPkg;
        apps.default = flake-utils.lib.mkApp {
          drv = ghostRecPkg;
        };
        apps.reaper-patcher = flake-utils.lib.mkApp {
          drv = reaperPatcherPkg;
          exePath = "/bin/reaper_patcher";
        };
      }
    );
}