{ pkgs ? import <nixpkgs> {} }:
  pkgs.stdenv.mkDerivation {
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
  }
