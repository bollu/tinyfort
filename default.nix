{ }:

let
  pkgs = import <nixpkgs> { };
in
  pkgs.stdenv.mkDerivation {
    name = "tinyfort-1.0.0";
    src = ./.;
    buildInputs = [ pkgs.cmake pkgs.flex pkgs.bison pkgs.boost pkgs.llvm_8
    pkgs.doxygen pkgs.graphviz pkgs.ninja pkgs.z3 pkgs.neovim pkgs.clang];
  }
