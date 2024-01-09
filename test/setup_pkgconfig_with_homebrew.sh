#!/bin/bash

# Homebrew does not put the .pc files in a common place that pkg-config can find.
# This script sets up the PKG_CONFIG_PATH so that the packages can be found by Natalie.
# You can run this prior to running `bin/natalie` or `rake test` or whatever.
#
# Usage:
#
#     eval(test/setup_pkgconfig_with_homebrew.sh)

set -e

packages=("libyaml")

if ! type -P brew &>/dev/null; then
  echo "Could not find $(brew) command. This script assumes you have Homebrew installed."
  exit 1
fi

for package in ${packages[@]}; do
  PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$(brew --prefix $package)/lib/pkgconfig"
done

echo "export PKG_CONFIG_PATH=\"$PKG_CONFIG_PATH\""
