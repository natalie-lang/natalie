{
  description = "Natalie - a Ruby compiler";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/0bb8272318b4ca41bf0edc6224df3e3e3046b70d";

    # Used by shell.nix for plain `nix-shell` support on stable Nix.
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };

    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            autoconf
            automake
            cacert
            ccache
            git
            gnumake
            libtool
            libffi
            libyaml
            openssl
            pkg-config
            ruby_4_0
            stdenv.cc
            zlib
          ];

          shellHook = ''
            export GEM_HOME="$PWD/.gem"
            unset GEM_PATH
            export PATH="$GEM_HOME/bin:$PATH"
          '';

          SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";
        };
      });
}
