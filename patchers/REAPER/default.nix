{ pkgs ? import <nixpkgs> {} }:
pkgs.python3Packages.buildPythonPackage {
  pname = "reaper-patcher";
  version = "0.1";
  src = ./.;
  format = "other";
  dontBuild = true;
  propagatedBuildInputs = [ pkgs.python3Packages.soundfile pkgs.python3Packages.numpy ];
  installPhase = ''
    mkdir -p $out/bin
    cp run.py $out/bin/reaper_patcher
    chmod +x $out/bin/reaper_patcher
  '';
}
